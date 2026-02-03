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

#include "../../cu_cp_impl_interface.h"
#include "../../mobility_manager/mobility_manager_impl.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

/// \brief Only observes target-side RRCReconfigurationComplete for this CHO candidate.
///
/// This routine is started on each prepared CHO target UE when the conditional
/// reconfiguration is sent to the source UE. It waits for target-side
/// RRCReconfigurationComplete or timeout.
///
class cho_target_routine
{
public:
  cho_target_routine(const cu_cp_cho_target_request& request_, ue_manager& ue_mng_, ocudulog::basic_logger& logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "CHO Target Routine"; }

private:
  const cu_cp_cho_target_request request;

  // Pointer to UE in the target DU.
  cu_cp_ue* target_ue = nullptr;

  ue_manager& ue_mng;

  ocudulog::basic_logger& logger;

  // (Sub-)routine result.
  bool reconf_result = false;
};

} // namespace ocucp
} // namespace ocudu
