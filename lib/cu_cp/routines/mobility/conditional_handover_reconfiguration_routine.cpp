// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "conditional_handover_reconfiguration_routine.h"
#include <algorithm>

using namespace ocudu;
using namespace ocudu::ocucp;

using namespace asn1::rrc_nr;

conditional_handover_reconfiguration_routine::conditional_handover_reconfiguration_routine(
    const cu_cp_cho_reconfiguration_request& request_,
    cu_cp_ue&                                source_ue_,
    f1ap_ue_context_manager&                 source_du_f1ap_ue_ctxt_mng_,
    cu_cp_ue_context_manipulation_handler&   cu_cp_handler_,
    cu_cp_ue_context_release_handler&        ue_context_release_handler_,
    ue_manager&                              ue_mng_,
    ocudulog::basic_logger&                  logger_) :
  request(request_),
  source_ue(source_ue_),
  source_du_f1ap_ue_ctxt_mng(source_du_f1ap_ue_ctxt_mng_),
  cu_cp_handler(cu_cp_handler_),
  ue_context_release_handler(ue_context_release_handler_),
  logger(logger_)
{
  ocudu_assert(source_ue.get_ue_index() != ue_index_t::invalid, "Invalid source UE index {}", source_ue.get_ue_index());
}

void conditional_handover_reconfiguration_routine::operator()(coro_context<async_task<bool>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", source_ue.get_ue_index(), name());

  // Get CHO context.
  {
    auto& cho_ctx = source_ue.get_cho_context();
    if (!cho_ctx.has_value() || cho_ctx->state != cu_cp_ue_cho_context::state_t::rrc_reconfiguration) {
      logger.warning("ue={}: \"{}\" failed. CHO not in rrc_reconfiguration phase", source_ue.get_ue_index(), name());
      CORO_EARLY_RETURN(false);
    }

    if (cho_ctx->candidates.empty()) {
      logger.warning("ue={}: \"{}\" failed. No candidates", source_ue.get_ue_index(), name());
      CORO_EARLY_RETURN(false);
    }
  }

  // Build the conditional reconfiguration message.
  rrc_container = build_conditional_reconfiguration_message();
  if (rrc_container.empty()) {
    logger.warning(
        "ue={}: \"{}\" failed. Could not build conditional reconfiguration", source_ue.get_ue_index(), name());
    CORO_AWAIT(cleanup_targets());
    CORO_EARLY_RETURN(false);
  }

  // Notify CU-CP to start waiting on each target UE for RRCReconfigurationComplete.
  {
    auto& cho_ctx = source_ue.get_cho_context();
    // Only one target observer per target UE index is required.
    // In intra-DU CHO all candidates map to source_ue_index, so exactly one conditional_handover_target_routine is
    // started.
    std::vector<ue_index_t> armed_target_ues;
    for (const auto& candidate : cho_ctx->candidates) {
      if (std::find(armed_target_ues.begin(), armed_target_ues.end(), candidate.target_ue_index) !=
          armed_target_ues.end()) {
        logger.debug("ue={}: CHO observer already started for target_ue={}. Skipping duplicate candidate "
                     "cond_recfg_id={} pci={} transaction_id={}",
                     source_ue.get_ue_index(),
                     candidate.target_ue_index,
                     candidate.cond_recfg_id,
                     candidate.target_pci,
                     candidate.rrc_reconfig_transaction_id);
        continue;
      }
      armed_target_ues.push_back(candidate.target_ue_index);

      cu_cp_cho_target_request target_request;
      target_request.source_ue_index = source_ue.get_ue_index();
      target_request.target_ue_index = candidate.target_ue_index;
      target_request.cond_recfg_id   = candidate.cond_recfg_id.value();
      target_request.target_pci      = candidate.target_pci;
      target_request.transaction_id  = candidate.rrc_reconfig_transaction_id;
      target_request.timeout         = request.timeout;

      // Pass bearer context modification request for CU-UP update after CHO completion.
      target_request.bearer_context_mod_request = candidate.bearer_context_mod_request;

      if (candidate.target_ue_index == source_ue.get_ue_index()) {
        logger.debug("ue={}: Starting intra-DU CHO observer on source UE for pci={} transaction_id={} "
                     "(cond_recfg_id={}); completion authority remains Access Success",
                     source_ue.get_ue_index(),
                     candidate.target_pci,
                     candidate.rrc_reconfig_transaction_id,
                     candidate.cond_recfg_id);
      } else {
        logger.debug("ue={}: Starting CHO target wait on target_ue={} pci={} transaction_id={}",
                     source_ue.get_ue_index(),
                     candidate.target_ue_index,
                     candidate.target_pci,
                     candidate.rrc_reconfig_transaction_id);
      }

      cu_cp_handler.handle_cho_reconfiguration_sent(target_request);
    }
  }

  // Generate the F1AP UE Context Modification request.
  generate_ue_context_modification_request();

  // Call F1AP procedure to send RRC reconfiguration to source UE via UE context modification request.
  CORO_AWAIT_VALUE(ue_context_mod_response,
                   source_du_f1ap_ue_ctxt_mng.handle_ue_context_modification_request(ue_context_mod_request));

  if (!ue_context_mod_response.success) {
    logger.warning("ue={}: \"{}\" failed. UE context modification failed", source_ue.get_ue_index(), name());
    CORO_AWAIT(cleanup_targets());
    CORO_EARLY_RETURN(false);
  }

  // Wait for RRCReconfigurationComplete on SOURCE UE (acknowledgment of conditional config).
  // The source RRC UE will await the completion with the transaction ID we used.
  CORO_AWAIT_VALUE(
      reconfig_result,
      source_ue.get_rrc_ue()->handle_handover_reconfiguration_complete_expected(transaction_id, request.timeout));

  if (!reconfig_result) {
    logger.warning(
        "ue={}: \"{}\" failed. RRCReconfigurationComplete not received on source", source_ue.get_ue_index(), name());
    CORO_AWAIT(cleanup_targets());
    CORO_EARLY_RETURN(false);
  }

  // Set CHO state to execution.
  {
    auto& cho_ctx = source_ue.get_cho_context();
    if (cho_ctx.has_value()) {
      cho_ctx->state = cu_cp_ue_cho_context::state_t::execution;
      logger.info("ue={}: CHO reconfiguration acknowledged. Waiting for UE to complete handover to target",
                  source_ue.get_ue_index());
    }
  }

  logger.debug("ue={}: \"{}\" finished successfully", source_ue.get_ue_index(), name());

  CORO_RETURN(true);
}

byte_buffer conditional_handover_reconfiguration_routine::build_conditional_reconfiguration_message()
{
  auto& cho_ctx = source_ue.get_cho_context();
  if (!cho_ctx.has_value() || cho_ctx->candidates.empty()) {
    logger.warning("ue={}: No CHO candidates available", source_ue.get_ue_index());
    return {};
  }

  // Get source RRC context for measurement config.
  rrc_ue_transfer_context source_rrc_context = source_ue.get_rrc_ue()->get_transfer_context();

  // Extract candidate target PCIs from CHO context.
  std::vector<pci_t> candidate_target_pcis;
  for (const auto& candidate : cho_ctx->candidates) {
    candidate_target_pcis.push_back(candidate.target_pci);
  }

  // Build the RRC reconfiguration request with CHO candidates and CHO-specific measurement config.
  rrc_reconfiguration_procedure_request rrc_request;
  rrc_request.cho_candidates = std::vector<cu_cp_ue_cho_candidate>();

  // Generate CHO-specific measurement config filtered for candidate targets.
  auto cho_meas_result =
      source_ue.get_rrc_ue()->generate_meas_config(source_rrc_context.meas_cfg, true, candidate_target_pcis);

  if (!cho_meas_result.has_value()) {
    logger.warning("ue={}: Failed to generate CHO measurement config", source_ue.get_ue_index());
    return {};
  }
  cho_meas_id_map_t cho_nci_to_meas_ids = cho_meas_result->nci_to_meas_ids;
  rrc_request.meas_cfg                  = std::move(cho_meas_result);
  rrc_request.cho_nci_to_meas_ids       = std::move(cho_nci_to_meas_ids);

  // Apply runtime T1 threshold override.
  if (request.t1_thres_override.has_value() && rrc_request.meas_cfg.has_value()) {
    for (auto& report_cfg_entry : rrc_request.meas_cfg->report_cfg_to_add_mod_list) {
      if (auto* cond = std::get_if<rrc_cond_trigger_cfg>(&report_cfg_entry.report_cfg)) {
        if (cond->cond_event_id.id == rrc_event_id::event_id_t::t1) {
          cond->cond_event_id.t1_thres = request.t1_thres_override;
        }
      }
    }
  }

  for (const auto& candidate : cho_ctx->candidates) {
    cu_cp_ue_cho_candidate cho_cand;
    cho_cand.cond_recfg_id      = candidate.cond_recfg_id;
    cho_cand.target_pci         = candidate.target_pci;
    cho_cand.target_cgi         = candidate.target_cgi; // Copy CGI from CU-CP layer
    cho_cand.prepared_rrc_recfg = candidate.prepared_rrc_recfg.copy();

    rrc_request.cho_candidates->push_back(std::move(cho_cand));

    logger.debug(
        "ue={}: Adding CHO candidate cond_recfg_id={} target_pci={} target_nci={:#x} prepared_rrc_recfg_size={}",
        source_ue.get_ue_index(),
        candidate.cond_recfg_id,
        candidate.target_pci,
        candidate.target_cgi.nci.value(),
        candidate.prepared_rrc_recfg.length());
  }

  // Get the CHO reconfiguration context from RRC UE.
  rrc_ue_cond_reconfiguration_context cond_reconf_ctxt =
      source_ue.get_rrc_ue()->get_rrc_ue_cond_reconfiguration_context(rrc_request);

  if (cond_reconf_ctxt.rrc_ue_cond_reconfiguration_pdu.empty()) {
    logger.warning("ue={}: Failed to get CHO reconfiguration context", source_ue.get_ue_index());
    return {};
  }

  transaction_id = cond_reconf_ctxt.transaction_id;

  logger.debug("ue={}: Built CHO RRCReconfiguration with {} candidate(s), transaction_id={}, pdu_size={}",
               source_ue.get_ue_index(),
               cho_ctx->candidates.size(),
               transaction_id,
               cond_reconf_ctxt.rrc_ue_cond_reconfiguration_pdu.length());

  return std::move(cond_reconf_ctxt.rrc_ue_cond_reconfiguration_pdu);
}

void conditional_handover_reconfiguration_routine::generate_ue_context_modification_request()
{
  ue_context_mod_request.ue_index      = source_ue.get_ue_index();
  ue_context_mod_request.rrc_container = rrc_container.copy();

  // For CHO, do NOT stop data transmission when sending the configuration.
  // Traffic should continue until the UE actually executes the handover (measurement conditions met).
  // Traffic stop/switch is handled later by CHO completion on Access Success.
}

async_task<void> conditional_handover_reconfiguration_routine::cleanup_targets()
{
  return launch_async([this](coro_context<async_task<void>>& ctx) {
    CORO_BEGIN(ctx);
    candidates_to_release.clear();
    if (source_ue.get_cho_context().has_value()) {
      for (const auto& candidate : source_ue.get_cho_context()->candidates) {
        // Release only externally prepared target UEs.
        // Intra-DU candidates reuse the source UE and must be kept.
        if (candidate.target_ue_index != ue_index_t::invalid && candidate.target_ue_index != source_ue.get_ue_index()) {
          candidates_to_release.push_back(candidate.target_ue_index);
        }
      }
      source_ue.get_cho_context()->clear();
    }
    release_idx = 0;
    while (release_idx < candidates_to_release.size()) {
      candidate_release_command.ue_index             = candidates_to_release[release_idx];
      candidate_release_command.cause                = ngap_cause_radio_network_t::unspecified;
      candidate_release_command.requires_rrc_release = false;
      CORO_AWAIT_VALUE(candidate_release_complete,
                       ue_context_release_handler.handle_ue_context_release_command(candidate_release_command));
      ++release_idx;
    }
    CORO_RETURN();
  });
}
