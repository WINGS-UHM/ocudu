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
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

/// \brief Cancels CHO if the UE never executes it after the execution timer fires.
///
/// Sends an RRCReconfiguration removing the conditional reconfig from the UE,
/// then releases all target UE contexts and clears the source CHO context.
class cho_cancellation_routine
{
public:
  cho_cancellation_routine(ue_index_t                        source_ue_index,
                           cu_cp_ue_context_release_handler& ue_context_release_handler,
                           ue_manager&                       ue_mng,
                           ocudulog::basic_logger&           logger);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "CHO Cancellation Routine"; }

private:
  const ue_index_t                  source_ue_index;
  cu_cp_ue_context_release_handler& ue_context_release_handler;
  ue_manager&                       ue_mng;
  ocudulog::basic_logger&           logger;

  cu_cp_ue*                             source_ue      = nullptr;
  bool                                  removal_result = false;
  rrc_reconfiguration_procedure_request removal_request;
  std::vector<ue_index_t>               targets_to_release;
  size_t                                release_idx = 0;
  cu_cp_ue_context_release_command      release_cmd;
  cu_cp_ue_context_release_complete     release_complete;
};

} // namespace ocucp
} // namespace ocudu
