/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cho_coordinator_routine.h"
#include "../../du_processor/du_processor_repository.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "cho_reconfiguration_routine.h"
#include "intra_cu_handover_routine.h"

using namespace ocudu;
using namespace ocudu::ocucp;

cho_coordinator_routine::cho_coordinator_routine(const cu_cp_intra_cu_cho_request& request_,
                                                 du_processor_repository&          du_db_,
                                                 cu_cp_impl_interface&             cu_cp_handler_,
                                                 ue_manager&                       ue_mng_,
                                                 mobility_manager&                 mobility_mng_,
                                                 ocudulog::basic_logger&           logger_) :
  request(request_),
  du_db(du_db_),
  cu_cp_handler(cu_cp_handler_),
  ue_mng(ue_mng_),
  mobility_mng(mobility_mng_),
  logger(logger_)
{
}

async_task<void> cho_coordinator_routine::release_prepared_targets()
{
  return launch_async(
      [this, idx = size_t{0}, result = cu_cp_ue_context_release_complete{}, cmd = cu_cp_ue_context_release_command{}](
          coro_context<async_task<void>>& ctx) mutable {
        CORO_BEGIN(ctx);
        for (idx = 0; idx < prepared_cho_targets.size(); ++idx) {
          cmd                      = {};
          cmd.ue_index             = prepared_cho_targets[idx].target_ue_index;
          cmd.cause                = ngap_cause_radio_network_t::unspecified;
          cmd.requires_rrc_release = false;
          CORO_AWAIT_VALUE(result, cu_cp_handler.handle_ue_context_release_command(cmd));
        }
        CORO_RETURN();
      });
}

void cho_coordinator_routine::operator()(coro_context<async_task<cu_cp_intra_cu_cho_response>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", request.source_ue_index, name());

  if (request.source_ue_index == ue_index_t::invalid || request.source_du_index == du_index_t::invalid ||
      request.targets.empty()) {
    logger.warning("CHO coordinator request is invalid");
    CORO_EARLY_RETURN(response);
  }

  source_ue = ue_mng.find_du_ue(request.source_ue_index);
  if (source_ue == nullptr || !source_ue->get_cho_context().has_value()) {
    logger.warning("ue={}: CHO coordinator failed. Source UE/CHO context missing", request.source_ue_index);
    CORO_EARLY_RETURN(response);
  }

  // Phase 1: CHO Preparation.
  source_ue->get_cho_context()->state = cu_cp_ue_cho_context::state_t::targets_preparation;
  for (candidate_idx = 0; candidate_idx < request.targets.size(); ++candidate_idx) {
    prep_request.source_ue_index = request.source_ue_index;
    prep_request.target_du_index = request.targets[candidate_idx].du_index;
    prep_request.target_pci      = request.targets[candidate_idx].pci;
    prep_request.cgi             = request.targets[candidate_idx].cgi;
    prep_request.cho_preparation.emplace();
    prep_request.cho_preparation->cond_recfg_id = cond_recfg_id;

    source_du_index  = request.source_du_index;
    target_du_index  = prep_request.target_du_index;
    target_cell_sib1 = du_db.get_du_processor(target_du_index).get_mobility_handler().get_packed_sib1(prep_request.cgi);
    CORO_AWAIT_VALUE(prep_response,
                     launch_async<intra_cu_handover_routine>(prep_request,
                                                             target_cell_sib1,
                                                             du_db.get_du_processor(source_du_index).get_f1ap_handler(),
                                                             du_db.get_du_processor(target_du_index).get_f1ap_handler(),
                                                             cu_cp_handler,
                                                             ue_mng,
                                                             mobility_mng,
                                                             logger));

    // Re-fetch source UE after every CORO_AWAIT, it may have been released during the async op.
    source_ue = ue_mng.find_du_ue(request.source_ue_index);
    if (source_ue == nullptr) {
      logger.warning("ue={}: Source UE removed during CHO preparation. Releasing {} prepared CHO target(s)",
                     request.source_ue_index,
                     prepared_cho_targets.size());
      CORO_AWAIT(release_prepared_targets());
      CORO_EARLY_RETURN(response);
    }

    if (!prep_response.success || !prep_response.cho_preparation_result.has_value()) {
      logger.warning("ue={}: CHO candidate preparation failed for target_pci={}",
                     request.source_ue_index,
                     request.targets[candidate_idx].pci);
    } else {
      cu_cp_cho_candidate candidate         = {};
      candidate.cond_recfg_id               = cond_recfg_id;
      candidate.target_pci                  = request.targets[candidate_idx].pci;
      candidate.target_cgi                  = request.targets[candidate_idx].cgi;
      candidate.target_ue_index             = prep_response.cho_preparation_result->target_ue_index;
      candidate.prepared_rrc_recfg          = std::move(prep_response.cho_preparation_result->packed_rrc_recfg);
      candidate.rrc_reconfig_transaction_id = prep_response.cho_preparation_result->transaction_id;
      candidate.bearer_context_mod_request.ng_ran_bearer_context_mod_request =
          std::move(prep_response.cho_preparation_result->ng_ran_bearer_context_mod_request);

      if (candidate.target_ue_index != request.source_ue_index) {
        prepared_cho_targets.push_back({candidate.target_ue_index, target_du_index});
      }

      if (source_ue->get_cho_context().has_value()) {
        source_ue->get_cho_context()->candidates.push_back(std::move(candidate));
      }
    }

    ++cond_recfg_id;
  }

  // Finalize CHO preparation state on source UE.
  source_ue = ue_mng.find_du_ue(request.source_ue_index);
  if (source_ue == nullptr || !source_ue->get_cho_context().has_value() ||
      source_ue->get_cho_context()->candidates.empty()) {
    logger.warning("ue={}: CHO coordinator failed. No prepared candidates", request.source_ue_index);
    CORO_EARLY_RETURN(response);
  }

  // Phase 2: CHO Execution.
  source_ue->get_cho_context()->state  = cu_cp_ue_cho_context::state_t::rrc_reconfiguration;
  cho_reconfig_request.source_ue_index = request.source_ue_index;
  cho_reconfig_request.timeout         = request.timeout;
  CORO_AWAIT_VALUE(
      cho_reconfig_result,
      launch_async<cho_reconfiguration_routine>(cho_reconfig_request,
                                                *source_ue,
                                                du_db.get_du_processor(request.source_du_index).get_f1ap_handler(),
                                                cu_cp_handler,
                                                cu_cp_handler,
                                                ue_mng,
                                                logger));
  if (!cho_reconfig_result) {
    logger.warning("ue={}: CHO coordinator failed. Reconfiguration phase failed", request.source_ue_index);
    CORO_EARLY_RETURN(response);
  }
  // Phase 3: CHO completion is handled asynchronously by cho_source_routine after Access Success.

  logger.debug("ue={}: \"{}\" finished successfully", source_ue->get_ue_index(), name());
  response.success = true;
  CORO_RETURN(response);
}
