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

#include "ocudu/asn1/e2ap/e2ap.h"
#include "ocudu/e2/e2.h"
#include "ocudu/e2/e2_event_manager.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/support/timers.h"

namespace ocudu {

class e2_connection_update_procedure
{
public:
  e2_connection_update_procedure(const asn1::e2ap::e2conn_upd_s& request_,
                                 e2_message_notifier&            ric_notif_,
                                 timer_factory                   timers_,
                                 ocudulog::basic_logger&         logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "E2 Connection Update Procedure"; }

private:
  // results senders
  void send_e2_connection_update_ack();
  void send_e2_connection_update_failure();

  const asn1::e2ap::e2conn_upd_s request;
  ocudulog::basic_logger&        logger;
  e2_message_notifier&           ric_notif;
  timer_factory                  timers;
};

} // namespace ocudu
