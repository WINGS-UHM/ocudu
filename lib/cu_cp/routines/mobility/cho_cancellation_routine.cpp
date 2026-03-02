// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "cho_cancellation_routine.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/ran/cause/ngap_cause.h"

using namespace ocudu;
using namespace ocudu::ocucp;

cho_cancellation_routine::cho_cancellation_routine(ue_index_t                        source_ue_index_,
                                                   cu_cp_ue_context_release_handler& ue_context_release_handler_,
                                                   ue_manager&                       ue_mng_,
                                                   ocudulog::basic_logger&           logger_) :
  source_ue_index(source_ue_index_),
  ue_context_release_handler(ue_context_release_handler_),
  ue_mng(ue_mng_),
  logger(logger_)
{
}

void cho_cancellation_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  source_ue = ue_mng.find_du_ue(source_ue_index);
  if (source_ue == nullptr) {
    CORO_EARLY_RETURN();
  }

  // Only proceed if CHO is still in the execution state.
  // cho_source_routine claims completion by transitioning to completion first.
  if (!source_ue->get_cho_context().has_value() ||
      source_ue->get_cho_context()->state != cu_cp_ue_cho_context::state_t::execution) {
    CORO_EARLY_RETURN();
  }

  // Claim ownership before any CORO_AWAIT to prevent cho_source_routine from racing.
  source_ue->get_cho_context()->state = cu_cp_ue_cho_context::state_t::completion;

  logger.info("ue={}: CHO execution timed out. Cancelling.", source_ue_index);

  // Build RRC removal request with condRecfgToRemList-r16.
  removal_request = {};
  removal_request.cho_cancellation_ids.emplace();
  for (const auto& candidate : source_ue->get_cho_context()->candidates) {
    if (candidate.cond_recfg_id.valid()) {
      removal_request.cho_cancellation_ids->push_back(candidate.cond_recfg_id.value());
    }
  }

  CORO_AWAIT_VALUE(removal_result, source_ue->get_rrc_ue()->handle_rrc_reconfiguration_request(removal_request));
  if (!removal_result) {
    logger.warning("ue={}: CHO cancellation RRC removal reconfig failed. Proceeding with target release.",
                   source_ue_index);
  }

  // Re-fetch source UE after CORO_AWAIT (it may have been released).
  source_ue = ue_mng.find_du_ue(source_ue_index);
  if (source_ue == nullptr) {
    CORO_EARLY_RETURN();
  }

  // Collect all inter-DU target UE indices to release.
  for (const auto& candidate : source_ue->get_cho_context()->candidates) {
    if (candidate.target_ue_index != ue_index_t::invalid && candidate.target_ue_index != source_ue_index) {
      targets_to_release.push_back(candidate.target_ue_index);
    }
  }

  // clear() stops the timer defensively and resets state to idle.
  source_ue->get_cho_context()->clear();

  // Release all target UE contexts.
  release_idx = 0;
  while (release_idx < targets_to_release.size()) {
    if (ue_mng.find_du_ue(targets_to_release[release_idx]) == nullptr) {
      ++release_idx;
      continue;
    }
    release_cmd                      = {};
    release_cmd.ue_index             = targets_to_release[release_idx];
    release_cmd.cause                = ngap_cause_radio_network_t::unspecified;
    release_cmd.requires_rrc_release = false;
    CORO_AWAIT_VALUE(release_complete, ue_context_release_handler.handle_ue_context_release_command(release_cmd));
    ++release_idx;
  }

  logger.info("ue={}: CHO cancellation complete.", source_ue_index);

  CORO_RETURN();
}
