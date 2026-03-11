// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../ue_context/xnap_ue_logger.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/cu_cp/inter_cu_handover_messages.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/support/async/protocol_transaction_manager.h"

namespace ocudu::ocucp {

/// Procedure used to await for the DL status
class xnap_sn_status_transfer_procedure
{
public:
  xnap_sn_status_transfer_procedure(
      protocol_transaction_event_source<asn1::xnap::sn_status_transfer_s>& sn_status_transfer_outcome_,
      xnap_ue_logger&                                                      logger_);

  void operator()(coro_context<async_task<expected<cu_cp_status_transfer>>>& ctx);

private:
  static const char* name() { return "SN Status Transfer Procedure"; }
  bool               fill_xnap_sn_status_transfer();

  protocol_transaction_event_source<asn1::xnap::sn_status_transfer_s>& sn_status_transfer_outcome;
  xnap_ue_logger&                                                      logger;

  ue_index_t            ue_index;
  cu_cp_status_transfer sn_status_transfer;

  protocol_transaction_outcome_observer<asn1::xnap::sn_status_transfer_s> transaction_sink;
};

} // namespace ocudu::ocucp
