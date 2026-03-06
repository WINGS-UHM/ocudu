// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../xnap_repository.h"
#include "ocudu/cu_cp/common_task_scheduler.h"
#include "ocudu/cu_cp/cu_cp_xnc_handler.h"

namespace ocudu::ocucp {

class cu_cp_routine_manager;
struct cu_cp_configuration;

class xnc_connection_manager : public cu_cp_xnc_handler
{
public:
  xnc_connection_manager(xnap_repository&       xnaps_,
                         task_executor&         cu_cp_exec_,
                         common_task_scheduler& common_task_sched_);

  void start();

  std::unique_ptr<xnap_message_notifier>
  handle_new_xnc_cu_cp_connection(std::unique_ptr<xnap_message_notifier> xnap_tx_pdu_notifier,
                                  const sctp_association_info&           assoc_info) override;

  void stop();

private:
  class shared_xnc_connection_context;
  class xnc_gw_to_cu_cp_pdu_adapter;

  void connect_to_neighbours();

  xnap_repository&        xnaps;
  task_executor&          cu_cp_exec;
  common_task_scheduler&  common_task_sched;
  ocudulog::basic_logger& logger;

  std::map<xnc_peer_index_t, std::shared_ptr<shared_xnc_connection_context>> xnc_connections;

  std::atomic<bool> stopped{false};
};

} // namespace ocudu::ocucp
