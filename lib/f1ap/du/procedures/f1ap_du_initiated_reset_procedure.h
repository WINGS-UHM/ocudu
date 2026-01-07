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

#include "f1ap_du_event_manager.h"
#include "ocudu/f1ap/du/f1ap_du.h"
#include "ocudu/f1ap/f1ap_message_notifier.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace odu {

class f1ap_du_ue_manager;

/// Implementation of the gNB-DU-initiated F1 RESET procedure as per TS 38.473, section 8.2.1.2.
class f1ap_du_initiated_reset_procedure
{
public:
  f1ap_du_initiated_reset_procedure(const f1_reset_request& req,
                                    f1ap_message_notifier&  msg_notifier,
                                    f1ap_event_manager&     ev_mng,
                                    f1ap_du_ue_manager&     ues);

  void operator()(coro_context<async_task<f1_reset_acknowledgement>>& ctx);

  const char* name() const { return "F1 Reset"; }

private:
  void                     send_f1_reset();
  f1_reset_acknowledgement handle_f1_reset_completion() const;

  const f1_reset_request  req;
  f1ap_message_notifier&  msg_notifier;
  f1ap_event_manager&     ev_mng;
  f1ap_du_ue_manager&     ues;
  ocudulog::basic_logger& logger;

  f1ap_transaction transaction;
};

} // namespace odu
} // namespace ocudu
