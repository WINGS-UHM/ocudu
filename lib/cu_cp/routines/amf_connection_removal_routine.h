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

async_task<void> start_amf_connection_removal(ngap_repository&                                    ngap_db,
                                              std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected);

/// \brief Handles the removal of the connection between the CU-CP and AMF.
class amf_connection_removal_routine
{
public:
  amf_connection_removal_routine(ngap_repository&                                    ngap_db_,
                                 std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected_);

  void operator()(coro_context<async_task<void>>& ctx);

private:
  std::unordered_map<amf_index_t, std::atomic<bool>>& amfs_connected;
  std::map<amf_index_t, ngap_interface*>              ngaps;
  ocudulog::basic_logger&                             logger;

  std::map<amf_index_t, ngap_interface*>::iterator ngap_it;
  amf_index_t                                      amf_index = amf_index_t::invalid;
  ngap_interface*                                  ngap      = nullptr;
};

} // namespace ocudu::ocucp
