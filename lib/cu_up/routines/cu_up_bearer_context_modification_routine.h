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

#include "../ue_context.h"
#include "ocudu/e1ap/cu_up/e1ap_cu_up_bearer_context_update.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocuup {

class cu_up_bearer_context_modification_routine
{
public:
  cu_up_bearer_context_modification_routine(ue_context& ue_ctxt, const e1ap_bearer_context_modification_request& msg);

  void operator()(coro_context<async_task<e1ap_bearer_context_modification_response>>& ctx);

  static const char* name() { return "CU-UP bearer context modification routine"; }

private:
  ue_context&                                     ue_ctxt;
  const e1ap_bearer_context_modification_request& msg;
  ocudulog::basic_logger&                         logger;

  // Helper variables
  e1ap_bearer_context_modification_response response = {};
  security::sec_as_config                   security_info;
};

} // namespace ocuup
} // namespace ocudu
