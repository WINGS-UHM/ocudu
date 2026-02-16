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

#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_configuration.h"
#include "ocudu/xnap/xnap_message.h"

namespace ocudu {
namespace ocucp {

class xn_setup_procedure
{
public:
  xn_setup_procedure(xnap_configuration xnap_cfg, xnc_connection_gateway& xnc_gw, ocudulog::basic_logger& logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "XN Setup Procedure"; }

private:
  void send_xn_setup_message();

  xnap_configuration      xnap_cfg;
  xnap_message            xn_setup_req;
  xnc_connection_gateway& xnc_gw;
  ocudulog::basic_logger& logger;
};

} // namespace ocucp
} // namespace ocudu
