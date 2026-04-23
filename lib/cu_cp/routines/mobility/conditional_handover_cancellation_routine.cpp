// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "conditional_handover_cancellation_routine.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/ran/cause/ngap_cause.h"

using namespace ocudu;
using namespace ocudu::ocucp;

conditional_handover_cancellation_routine::conditional_handover_cancellation_routine(ue_index_t  source_ue_index_,
                                                                                     ue_manager& ue_mng_,
                                                                                     ocudulog::basic_logger& logger_) :
  source_ue_index(source_ue_index_), ue_mng(ue_mng_), logger(logger_)
{
}

void conditional_handover_cancellation_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  source_ue = ue_mng.find_du_ue(source_ue_index);
  if (source_ue == nullptr) {
    CORO_EARLY_RETURN();
  }

  // Only proceed if CHO is still in the execution state.
  // conditional_handover_source_routine claims completion by transitioning to completion first.
  if (!source_ue->get_cho_context().has_value() ||
      source_ue->get_cho_context()->state != cu_cp_ue_cho_context::state_t::execution) {
    CORO_EARLY_RETURN();
  }

  // Claim ownership before any CORO_AWAIT to prevent conditional_handover_source_routine from racing.
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

  // Cancel each candidate's RRC reconfiguration transaction.
  unsigned cancelled = 0;
  for (const auto& candidate : source_ue->get_cho_context()->candidates) {
    if (candidate.target_ue_index == ue_index_t::invalid || candidate.target_ue_index == source_ue_index) {
      continue;
    }
    auto* candidate_ue = ue_mng.find_du_ue(candidate.target_ue_index);
    if (candidate_ue == nullptr) {
      continue;
    }
    candidate_ue->get_rrc_ue()->cancel_handover_reconfiguration_transaction(
        static_cast<uint8_t>(candidate.rrc_reconfig_transaction_id));
    ++cancelled;
  }

  // clear() stops the timer defensively and resets state to idle.
  source_ue->get_cho_context()->clear();

  logger.info("ue={}: CHO cancellation complete. {} candidates cancelled.", source_ue_index, cancelled);

  CORO_RETURN();
}
