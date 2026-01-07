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

#include "../../ue_manager/cu_cp_ue_impl.h"
#include "ocudu/e1ap/cu_cp/e1ap_cu_cp.h"
#include "ocudu/e1ap/cu_cp/e1ap_cu_cp_bearer_context_update.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/ngap/ngap_handover.h"
#include "ocudu/support/async/async_task.h"
#include <chrono>

namespace ocudu::ocucp {

/// Routine for the target gNB execution phase of the handover,
/// as specified in TS 23.502, section 4.9.1.3.3.
class inter_cu_handover_execution_target_routine
{
public:
  inter_cu_handover_execution_target_routine(cu_cp_ue*                    ue_,
                                             e1ap_bearer_context_manager& e1ap_,
                                             ngap_interface&              ngap_,
                                             ocudulog::basic_logger&      logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "Inter CU Handover Execution Target Routine"; }

private:
  void fill_e1ap_bearer_context_modification_request();
  bool initialize_reconfiguration_timeout();

  // (sub-)routine requests
  e1ap_bearer_context_modification_request bearer_context_modification_request;

  // (sub-)routine results
  expected<ngap_dl_ran_status_transfer>     ngap_dl_ran_status;
  e1ap_bearer_context_modification_response bearer_context_modification_response;

  cu_cp_ue*                    ue = nullptr;
  e1ap_bearer_context_manager& e1ap;
  ngap_interface&              ngap;
  ocudulog::basic_logger&      logger;

  std::chrono::milliseconds reconf_timeout;
  bool                      reconf_result = false;
};

} // namespace ocudu::ocucp
