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

#include "../ue_context/ngap_ue_context.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/ngap/ngap_nas.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

class ngap_dl_nas_message_transfer_procedure
{
public:
  ngap_dl_nas_message_transfer_procedure(const ngap_dl_nas_transport_message&         msg_,
                                         ngap_rrc_ue_notifier&                        rrc_ue_notifier_,
                                         ngap_ue_radio_capability_management_handler& ngap_handler_,
                                         ngap_ue_logger&                              logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "DL NAS Message Transfer Procedure"; }

private:
  // results senders
  void send_pdu_to_rrc_ue();
  void send_ue_radio_capability_info_indication();

  ngap_dl_nas_transport_message                msg;
  ngap_rrc_ue_notifier&                        rrc_ue_notifier;
  ngap_ue_radio_capability_management_handler& ngap_handler;
  ngap_ue_logger&                              logger;
};

} // namespace ocucp
} // namespace ocudu
