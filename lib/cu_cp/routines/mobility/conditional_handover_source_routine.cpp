// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "conditional_handover_source_routine.h"

using namespace ocudu;
using namespace ocudu::ocucp;

conditional_handover_source_routine::conditional_handover_source_routine(const cu_cp_access_success_indication& msg_,
                                                                         ue_manager&                            ue_mng_,
                                                                         ocudulog::basic_logger& logger_) :
  msg(msg_), ue_mng(ue_mng_), logger(logger_)
{
}

void conditional_handover_source_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  source_ue = ue_mng.find_du_ue(msg.source_ue_index);
  if (source_ue == nullptr) {
    logger.warning("source_ue={}: UE no longer exists at start of CHO source routine", msg.source_ue_index);
    CORO_EARLY_RETURN();
  }

  if (!source_ue->get_cho_context().has_value()) {
    logger.warning("source_ue={}: Access Success received but no CHO context", msg.source_ue_index);
    CORO_EARLY_RETURN();
  }

  // Access Success arrived, normal completion takes over. Stop CHO execution timer.
  source_ue->get_cho_context()->cho_execution_timer.stop();

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

  logger.info("source_ue={}: CHO winner selected target_ue={} target_cgi={}",
              msg.source_ue_index,
              winner->target_ue_index,
              winner->target_cgi.nci);

  // Cancel each non-winning candidate's RRC reconfiguration transaction. Each non-winner's target routine observes the
  // cancellation and self-releases.
  unsigned cancelled = 0;
  for (const auto& candidate : source_ue->get_cho_context()->candidates) {
    if (candidate.target_ue_index == ue_index_t::invalid || candidate.target_ue_index == source_ue->get_ue_index() ||
        candidate.target_ue_index == winner->target_ue_index) {
      continue;
    }
    auto* loser_ue = ue_mng.find_du_ue(candidate.target_ue_index);
    if (loser_ue == nullptr) {
      logger.debug("source_ue={}: Skipping cancel for already-removed CHO candidate target_ue={}",
                   msg.source_ue_index,
                   candidate.target_ue_index);
      continue;
    }
    loser_ue->get_rrc_ue()->cancel_handover_reconfiguration_transaction(
        static_cast<uint8_t>(candidate.rrc_reconfig_transaction_id));
    ++cancelled;
  }

  // Clear source CHO context. The winning target UE's CHO context is cleared by the target routine.
  source_ue->get_cho_context()->clear();

  logger.info(
      "source_ue={}: CHO source cancelled {} non-winning candidates on Access Success", msg.source_ue_index, cancelled);

  CORO_RETURN();
}
