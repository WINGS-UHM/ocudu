/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ue_context/ngap_ue_logger.h"
#include "ue_context/ngap_ue_transaction_manager.h"
#include "ocudu/ngap/ngap_handover.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

/// Procedure used to await for the DL status
class ngap_dl_ran_status_transfer_procedure
{
public:
  ngap_dl_ran_status_transfer_procedure(ngap_ue_transaction_manager& ev_mng,
                                        timer_factory                timers,
                                        ngap_ue_logger&              logger_);

  void operator()(coro_context<async_task<expected<ngap_dl_ran_status_transfer>>>& ctx);

private:
  static const char* name() { return "DL RAN Status Transfer Procedure"; }
  bool               fill_ngap_dl_ran_status_transfer();

  ngap_ue_transaction_manager& ev_mng;
  ngap_ue_logger&              logger;

  ue_index_t                  ue_index;
  ngap_dl_ran_status_transfer dl_ran_status_transfer;

  protocol_transaction_outcome_observer<asn1::ngap::dl_ran_status_transfer_s> transaction_sink;
  protocol_transaction_outcome_observer<asn1::ngap::ho_cancel_ack_s> dl_status_transfer_cancel_transaction_sink;
};

} // namespace ocudu::ocucp
