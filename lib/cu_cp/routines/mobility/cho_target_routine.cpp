// Copyright 2021-2026 Software Radio Systems Limited
// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "cho_target_routine.h"

using namespace ocudu;
using namespace ocudu::ocucp;

cho_target_routine::cho_target_routine(const cu_cp_cho_target_request& request_,
                                       ue_manager&                     ue_mng_,
                                       ocudulog::basic_logger&         logger_) :
  request(request_), ue_mng(ue_mng_), logger(logger_)
{
}

void cho_target_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  if (ue_mng.find_du_ue(request.target_ue_index) == nullptr) {
    logger.warning("CHO target_ue={} got removed", request.target_ue_index);
    CORO_EARLY_RETURN();
  }
  target_ue = ue_mng.find_du_ue(request.target_ue_index);

  logger.debug(
      "target_ue={} source_ue={}: \"{}\" started. Waiting for RRCReconfigurationComplete with transaction_id={}",
      request.target_ue_index,
      request.source_ue_index,
      name(),
      request.transaction_id);

  // Wait for RRCReconfigurationComplete on this target UE.
  // The UE will send this after it evaluates the CHO condition and attaches to this target.
  // Use the transaction ID from the prepared RRC reconfiguration for this candidate.
  CORO_AWAIT_VALUE(reconf_result,
                   target_ue->get_rrc_ue()->handle_handover_reconfiguration_complete_expected(request.transaction_id,
                                                                                              request.timeout));

  if (!reconf_result) {
    logger.debug("target_ue={}: \"{}\" did not receive RRCReconfigurationComplete. CHO to this target failed/canceled",
                 request.target_ue_index,
                 name());
    CORO_EARLY_RETURN();
  }

  logger.debug("target_ue={}: \"{}\" observed RRCReconfigurationComplete. Completion will be finalized on Access "
               "Success",
               request.target_ue_index,
               name());

  CORO_RETURN();
}
