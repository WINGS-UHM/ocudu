// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../../cu_cp_impl_interface.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

/// \brief Releases non-winning CHO candidates upon ACCESS SUCCESS.
///
/// Runs on the source UE's task scheduler concurrently with the target routine. Responsible for source-side cleanup:
/// identifying the winner, releasing non-winning candidates, and clearing the source CHO context. Source UE release is
/// done in the target routine.
class conditional_handover_source_routine
{
public:
  conditional_handover_source_routine(const cu_cp_access_success_indication& msg_,
                                      ue_manager&                            ue_mng_,
                                      cu_cp_ue_context_release_handler&      ue_context_release_handler_,
                                      ocudulog::basic_logger&                logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "CHO Source Routine"; }

private:
  const cu_cp_access_success_indication msg;

  ue_manager&                       ue_mng;
  cu_cp_ue_context_release_handler& ue_context_release_handler;
  ocudulog::basic_logger&           logger;

  cu_cp_ue* source_ue = nullptr;

  cu_cp_cho_candidate* winner = nullptr;

  std::vector<ue_index_t> inter_du_targets_to_release;
  size_t                  release_idx = 0;

  cu_cp_ue_context_release_command  release_cmd;
  cu_cp_ue_context_release_complete release_complete;
};

} // namespace ocucp
} // namespace ocudu
