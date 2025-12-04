/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "../ngap_repository.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

async_task<bool> start_amf_connection_setup(ngap_repository&                                    ngap_db,
                                            std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected);

/// \brief Handles the setup of the connection between the CU-CP and AMF, handling in particular the NG Setup procedure.
class amf_connection_setup_routine
{
public:
  amf_connection_setup_routine(ngap_repository& ngap_db_, std::atomic<bool>& amf_connected_);

  void operator()(coro_context<async_task<bool>>& ctx);

private:
  void handle_connection_setup_result();

  ngap_repository&        ngap_db;
  std::atomic<bool>&      amf_connected;
  amf_index_t             amf_index = amf_index_t::invalid;
  ngap_interface*         ngap      = nullptr;
  ocudulog::basic_logger& logger;

  ngap_ng_setup_result result_msg = {};
  bool                 success    = false;
};

} // namespace ocucp
} // namespace ocudu
