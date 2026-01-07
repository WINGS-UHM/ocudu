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

#include "../ngap_repository.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

async_task<bool> start_amf_connection_setup(ngap_repository&                                    ngap_db,
                                            std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected);

/// \brief Handles the setup of the connection between the CU-CP and AMF, handling in particular the NG Setup procedure.
class amf_connection_setup_routine
{
public:
  amf_connection_setup_routine(ngap_repository&                                    ngap_db_,
                               std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected_);

  void operator()(coro_context<async_task<bool>>& ctx);

private:
  void handle_connection_setup_result();

  ngap_repository&                                    ngap_db;
  std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected;
  std::map<amf_index_t, ngap_interface*>              ngaps;
  ocudulog::basic_logger&                             logger;

  std::map<amf_index_t, ngap_interface*>::iterator ngap_it;
  amf_index_t                                      amf_index = amf_index_t::invalid;
  ngap_interface*                                  ngap      = nullptr;

  ngap_ng_setup_result result_msg = {};
  bool                 success    = false;
};

} // namespace ocudu::ocucp
