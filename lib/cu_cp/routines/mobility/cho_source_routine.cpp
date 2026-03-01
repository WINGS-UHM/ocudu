// Copyright 2021-2026 Software Radio Systems Limited
// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "cho_source_routine.h"
#include "../../cu_up_processor/cu_up_processor_repository.h"
#include "../../du_processor/du_processor_repository.h"
#include "../../mobility_manager/mobility_manager_impl.h"

using namespace ocudu;
using namespace ocudu::ocucp;

cho_source_routine::cho_source_routine(const cu_cp_access_success_indication& msg_,
                                       ue_manager&                            ue_mng_,
                                       du_processor_repository&               du_db_,
                                       cu_up_processor_repository&            cu_up_db_,
                                       cu_cp_ue_context_manipulation_handler& cu_cp_handler_,
                                       cu_cp_ue_context_release_handler&      ue_context_release_handler_,
                                       mobility_manager&                      mobility_mng_,
                                       ocudulog::basic_logger&                logger_) :
  msg(msg_),
  ue_mng(ue_mng_),
  du_db(du_db_),
  cu_up_db(cu_up_db_),
  cu_cp_handler(cu_cp_handler_),
  ue_context_release_handler(ue_context_release_handler_),
  mobility_mng(mobility_mng_),
  logger(logger_)
{
}

void cho_source_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  if (!resolve_source_ue()) {
    CORO_EARLY_RETURN();
  }
  source_ue_index = source_ue->get_ue_index();

  // Access Success arrived, normal completion takes over.Stop CHO execution timer.
  if (source_ue->get_cho_context().has_value()) {
    source_ue->get_cho_context()->cho_execution_timer.stop();
  }

  winner = source_ue->get_cho_context()->find_candidate(msg.cgi);
  if (winner == nullptr) {
    winner = source_ue->get_cho_context()->find_candidate(msg.ue_index);
  }
  if (winner == nullptr) {
    logger.warning("ue={}: Access Success (cgi={}) does not match any CHO candidate for source_ue={}",
                   msg.ue_index,
                   msg.cgi.nci,
                   source_ue->get_ue_index());
    CORO_EARLY_RETURN();
  }

  source_ue->get_cho_context()->state = cu_cp_ue_cho_context::state_t::completion;
  is_inter_du_winner                  = (winner->target_ue_index != source_ue->get_ue_index());
  // Note: currently no support for intra-DU CHO, and all CHOs are triggered as in inter-DU case (i.e., with new UE
  // context allocation), hence is_inter_du_winner is always true. CHO Source Routine is ready to handle both cases.

  logger.info("source_ue={}: CHO winner selected target_ue={} target_cgi={}",
              source_ue_index,
              winner->target_ue_index,
              winner->target_cgi.nci);

  // Inter-DU winner finalization.
  if (is_inter_du_winner) {
    target_ue = ue_mng.find_du_ue(winner->target_ue_index);
    if (target_ue == nullptr) {
      logger.warning("source_ue={}: Access Success winner target_ue={} no longer exists",
                     source_ue_index,
                     winner->target_ue_index);
      CORO_EARLY_RETURN();
    }

    cu_cp_handler.handle_handover_ue_context_push(source_ue_index, winner->target_ue_index);

    bearer_ctx_mod_request = winner->bearer_context_mod_request;
    if (!fill_bearer_context_security_info(bearer_ctx_mod_request,
                                           target_ue->get_security_manager().get_up_as_config())) {
      CORO_EARLY_RETURN();
    }
    bearer_ctx_mod_request.ue_index = winner->target_ue_index;

    CORO_AWAIT_VALUE(bearer_ctx_mod_response,
                     cu_up_db.find_cu_up_processor(target_ue->get_cu_up_index())
                         ->get_e1ap_bearer_context_manager()
                         .handle_bearer_context_modification_request(bearer_ctx_mod_request));
    if (!bearer_ctx_mod_response.success) {
      logger.warning("source_ue={} target_ue={}: CU-UP bearer context modification failed",
                     source_ue_index,
                     winner->target_ue_index);
      CORO_EARLY_RETURN();
    }

    target_du_context_mod_request.ue_index               = winner->target_ue_index;
    target_du_context_mod_request.rrc_recfg_complete_ind = f1ap_rrc_recfg_complete_ind::true_value;
    CORO_AWAIT(du_db.get_du_processor(target_ue->get_du_index())
                   .get_f1ap_handler()
                   .handle_ue_context_modification_request(target_du_context_mod_request));
  }

  // Release all non-winning inter-DU candidates.
  for (const auto& candidate : source_ue->get_cho_context()->candidates) {
    if (candidate.target_ue_index != ue_index_t::invalid && candidate.target_ue_index != source_ue->get_ue_index() &&
        candidate.target_ue_index != winner->target_ue_index) {
      if (std::find(inter_du_targets_to_release.begin(),
                    inter_du_targets_to_release.end(),
                    candidate.target_ue_index) == inter_du_targets_to_release.end()) {
        inter_du_targets_to_release.push_back(candidate.target_ue_index);
      }
    }
  }

  // Clear CHO context on both the source and the winning target UE.
  // Target is reset first so winner->target_ue_index is still valid before candidates are cleared.
  if (auto* winning_ue = ue_mng.find_du_ue(winner->target_ue_index); winning_ue != nullptr) {
    winning_ue->get_cho_context().reset();
  }
  source_ue->get_cho_context()->clear();
  mobility_mng.get_metrics_handler().aggregate_successful_handover_execution();

  release_idx = 0;
  while (release_idx < inter_du_targets_to_release.size()) {
    if (ue_mng.find_du_ue(inter_du_targets_to_release[release_idx]) == nullptr) {
      logger.debug("source_ue={}: Skipping release for already-removed CHO candidate target_ue={}",
                   source_ue_index,
                   inter_du_targets_to_release[release_idx]);
      ++release_idx;
      continue;
    }
    release_cmd                      = {};
    release_cmd.ue_index             = inter_du_targets_to_release[release_idx];
    release_cmd.cause                = ngap_cause_radio_network_t::unspecified;
    release_cmd.requires_rrc_release = false;
    CORO_AWAIT_VALUE(release_complete, ue_context_release_handler.handle_ue_context_release_command(release_cmd));
    ++release_idx;
  }

  // For inter-DU winner, release source UE context after cleanup.
  if (is_inter_du_winner && ue_mng.find_du_ue(source_ue_index) != nullptr) {
    release_cmd                      = {};
    release_cmd.ue_index             = source_ue_index;
    release_cmd.cause                = ngap_cause_radio_network_t::unspecified;
    release_cmd.requires_rrc_release = false;
    CORO_AWAIT_VALUE(release_complete, ue_context_release_handler.handle_ue_context_release_command(release_cmd));
  }

  logger.info("source_ue={}: CHO completion finalized via Access Success", source_ue_index);

  CORO_RETURN();
}

bool cho_source_routine::fill_bearer_context_security_info(e1ap_bearer_context_modification_request& request,
                                                           const security::sec_as_config&            sec_cfg)
{
  request.security_info.emplace();
  request.security_info->security_algorithm.ciphering_algo                 = sec_cfg.cipher_algo;
  request.security_info->security_algorithm.integrity_protection_algorithm = sec_cfg.integ_algo;

  auto k_enc = byte_buffer::create(sec_cfg.k_enc);
  if (!k_enc.has_value()) {
    logger.warning("source_ue={}: Failed to allocate UP security encryption key", source_ue->get_ue_index());
    return false;
  }
  request.security_info->up_security_key.encryption_key = std::move(k_enc.value());

  if (sec_cfg.k_int.has_value()) {
    auto k_int = byte_buffer::create(sec_cfg.k_int.value());
    if (!k_int.has_value()) {
      logger.warning("source_ue={}: Failed to allocate UP security integrity key", source_ue->get_ue_index());
      return false;
    }
    request.security_info->up_security_key.integrity_protection_key = std::move(k_int.value());
  }

  return true;
}

bool cho_source_routine::resolve_source_ue()
{
  source_ue = ue_mng.find_du_ue(msg.ue_index);
  if (source_ue == nullptr) {
    logger.warning("ue={}: Access Success received but UE context not found", msg.ue_index);
    return false;
  }

  // Access Success can arrive on either source or target UE-associated signalling context.
  // If this UE is tagged as a target candidate, look up the source UE directly via backlink.
  if (source_ue->get_cho_context().has_value() &&
      source_ue->get_cho_context()->role == cu_cp_ue_cho_context::role_t::target &&
      source_ue->get_cho_context()->source_ue_index != ue_index_t::invalid) {
    source_ue = ue_mng.find_du_ue(source_ue->get_cho_context()->source_ue_index);
  }

  if (source_ue == nullptr || !source_ue->get_cho_context().has_value() ||
      source_ue->get_cho_context()->state == cu_cp_ue_cho_context::state_t::idle) {
    logger.debug("ue={}: Access Success received but no active CHO context", msg.ue_index);
    return false;
  }

  return true;
}
