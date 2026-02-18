/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cell_deactivation_routine.h"
#include "../du_processor/du_processor_repository.h"
#include "ocudu/f1ap/cu_cp/f1ap_cu_configuration_update.h"
#include "ocudu/ran/cause/ngap_cause.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/support/async/coroutine.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::rrc_nr;

cell_deactivation_routine::cell_deactivation_routine(const cu_cp_configuration&        cu_cp_cfg_,
                                                     const std::vector<plmn_identity>& plmns_,
                                                     du_processor_repository&          du_db_,
                                                     cu_cp_ue_context_release_handler& ue_release_handler_,
                                                     ue_manager&                       ue_mng_,
                                                     ocudulog::basic_logger&           logger_) :
  cu_cp_cfg(cu_cp_cfg_),
  plmns(plmns_),
  du_db(du_db_),
  ue_release_handler(ue_release_handler_),
  ue_mng(ue_mng_),
  logger(logger_)
{
}

void cell_deactivation_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.info("\"{}\" started...", name());
  proc_start_tp = std::chrono::steady_clock::now();

  // Send F1AP UE Context Release Command for all UEs with the PLMNs served by the disconnected AMF.
  trigger_context_release_for_all_ues();

  // Wait until all UEs are released. The release tasks were eagerly scheduled onto each UE's FIFO
  // task scheduler in trigger_context_release_for_all_ues(), so all releases run in parallel.
  for (ue_release_task_it = ue_release_tasks.begin(); ue_release_task_it != ue_release_tasks.end();
       ++ue_release_task_it) {
    CORO_AWAIT(*ue_release_task_it);
  }
  ue_release_finish_tp = std::chrono::steady_clock::now();
  logger.debug("\"{}\" all UEs released, duration: {:.3f} seconds",
               name(),
               std::chrono::duration<double>(ue_release_finish_tp - proc_start_tp).count());

  // Deactivate all cells that serve this PLMN.
  du_indexes = du_db.get_du_processor_indexes();
  for (du_idx_it = du_indexes.begin(); du_idx_it != du_indexes.end(); ++du_idx_it) {
    du_proc = du_db.find_du_processor(*du_idx_it);
    if (du_proc == nullptr) {
      logger.warning("DU processor not found for index {}", *du_idx_it);
      continue;
    }

    if (!generate_gnb_cu_configuration_update()) {
      logger.warning("Failed to generate gNB-CU configuration update for du={}", *du_idx_it);
      continue;
    }

    CORO_AWAIT_VALUE(f1ap_cu_cfg_update_response, du_proc->handle_configuration_update(f1ap_cu_cfg_update));
    if (!f1ap_cu_cfg_update_response.success) {
      logger.info(
          "Configuration update for du={} failed. Cause: {}", *du_idx_it, f1ap_cu_cfg_update_response.cause.value());
      routine_success = false;
    }
  }

  if (routine_success) {
    logger.info("\"{}\" finished successfully, duration: {:.3f} seconds",
                name(),
                std::chrono::duration<double>(std::chrono::steady_clock::now() - proc_start_tp).count());
  } else {
    logger.info("\"{}\" failed, duration: {:.3f} seconds",
                name(),
                std::chrono::duration<double>(std::chrono::steady_clock::now() - proc_start_tp).count());
  }

  CORO_RETURN();
}

void cell_deactivation_routine::trigger_context_release_for_all_ues()
{
  // Release all UEs with the PLMNs served by the disconnected AMF.
  // Each release task is dispatched onto the UE's own FIFO task scheduler via dispatch_and_await_task_completion,
  // then wrapped in ue_release_task_t an eager_async_task so the scheduling happens immediately (not deferred until
  // awaited). This way all releases should be started in parallel.
  for (const auto& plmn : plmns) {
    std::vector<cu_cp_ue*> ues = ue_mng.find_ues(plmn);

    for (const auto& ue : ues) {
      if (ue != nullptr) {
        logger.info("ue={}: Releasing UE (PLMN {}) due to N2 disconnection", ue->get_ue_index(), plmn);
        cu_cp_ue_context_release_command command{
            ue->get_ue_index(), ngap_cause_transport_t::transport_res_unavailable, true, std::chrono::seconds{5}};

        auto release = ue_mng.get_task_sched().dispatch_and_await_task_completion(
            ue->get_ue_index(), ue_release_handler.handle_ue_context_release_command(command));
        ue_release_tasks.push_back(
            launch_async([release = std::move(release)](coro_context<ue_release_task_t>& ctx) mutable {
              CORO_BEGIN(ctx);
              CORO_AWAIT_VALUE(auto result, release);
              CORO_RETURN(result);
            }));
      }
    }
  }
}

void cell_deactivation_routine::get_remaining_plmns(const du_cell_configuration& cell_cfg)
{
  // Check which of the cells of this DU serve the lost PLMNs.
  std::set<plmn_identity> lost_plmns(plmns.begin(), plmns.end());

  for (const auto& served_plmn : cell_cfg.served_plmns) {
    // If the PLMN is not in the lost PLMNs, we need to keep it.
    if (!lost_plmns.count(served_plmn)) {
      remaining_plmns.insert(served_plmn);
    }
  }
}

bool cell_deactivation_routine::generate_gnb_cu_configuration_update()
{
  const du_configuration_context* du_ctxt = du_proc->get_context();
  if (du_ctxt == nullptr) {
    logger.warning("DU context not found for index {}", *du_idx_it);
    return false;
  }

  f1ap_cu_cfg_update = {};

  for (const auto& served_cell : du_ctxt->served_cells) {
    // Check which of the cells of this DU serve the lost PLMNs.
    get_remaining_plmns(served_cell);

    if (remaining_plmns.empty()) {
      // All PLMNs are lost, so we can deactivate the cell.
      f1ap_cu_cfg_update.cells_to_be_deactivated_list.push_back({served_cell.cgi});
      continue;
    }

    if (remaining_plmns.size() < served_cell.served_plmns.size()) {
      f1ap_cu_cfg_update.cells_to_be_deactivated_list.push_back({served_cell.cgi});
      // TODO: Deactivate only specific PLMNs
    }
  }

  f1ap_cu_cfg_update.gnb_cu_name = cu_cp_cfg.node.ran_node_name;

  return true;
}
