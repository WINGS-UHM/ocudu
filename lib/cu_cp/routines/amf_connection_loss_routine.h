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

#include "../cu_cp_controller/cu_cp_controller.h"
#include "../ue_manager/ue_manager_impl.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/support/timers.h"

namespace ocudu {
namespace ocucp {

/// \brief Handles the reconnection between the CU-CP and AMF.
class amf_connection_loss_routine
{
public:
  amf_connection_loss_routine(const amf_index_t                 amf_index_,
                              const cu_cp_configuration&        cu_cp_cfg_,
                              std::vector<plmn_identity>&       plmns_,
                              du_processor_repository&          du_db_,
                              cu_cp_ue_context_release_handler& ue_release_handler_,
                              ue_manager&                       ue_mng_,
                              cu_cp_controller&                 controller_,
                              ocudulog::basic_logger&           logger_);

  static std::string name() { return "AMF Connection Loss Routine"; }

  void operator()(coro_context<async_task<void>>& ctx);

private:
  const amf_index_t                 amf_index;
  const cu_cp_configuration&        cu_cp_cfg;
  std::vector<plmn_identity>&       plmns;
  du_processor_repository&          du_db;
  cu_cp_ue_context_release_handler& ue_release_handler;
  ue_manager&                       ue_mng;
  cu_cp_controller&                 controller;
  ocudulog::basic_logger&           logger;
};

} // namespace ocucp
} // namespace ocudu
