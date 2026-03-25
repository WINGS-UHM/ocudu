// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../xnap_repository.h"
#include "ocudu/cu_cp/common_task_scheduler.h"
#include "ocudu/cu_cp/cu_cp_xnc_handler.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"
#include "ocudu/xnap/xnap_configuration.h"
#include <condition_variable>
#include <mutex>

namespace ocudu::ocucp {

class cu_cp_routine_manager;
struct cu_cp_configuration;

class xnc_connection_manager : public cu_cp_xnc_handler
{
public:
  xnc_connection_manager(xnap_repository&        xnaps_,
                         xnc_connection_gateway* xnc_gw_,
                         task_executor&          cu_cp_exec_,
                         common_task_scheduler&  common_task_sched_);

  void start(const xnap_configuration& xnap_cfg);

  std::unique_ptr<xnap_message_notifier>
  handle_new_xnc_cu_cp_connection(std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier,
                                  const sctp_association_info&           assoc_info) override;

  void handle_xnc_gw_connection_closed(xnc_peer_index_t xnc_idx);

  void stop();

private:
  class shared_xnc_connection_context;
  class xnc_gw_to_cu_cp_pdu_adapter;

  xnap_repository&        xnaps;
  xnc_connection_gateway* xnc_gw;
  task_executor&          cu_cp_exec;
  common_task_scheduler&  common_task_sched;
  ocudulog::basic_logger& logger;

  /// XNAP configuration used to recreate XNAP instances after connection loss.
  xnap_configuration xnap_cfg;

  std::map<xnc_peer_index_t, std::shared_ptr<shared_xnc_connection_context>> xnc_connections;

  std::atomic<bool>       stopped{false};
  std::mutex              stop_mutex;
  std::condition_variable stop_cvar;
  bool                    stop_completed = false;
};

} // namespace ocudu::ocucp
