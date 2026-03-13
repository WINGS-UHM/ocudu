// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../ue_context/xnap_ue_context.h"
#include "ocudu/cu_cp/inter_cu_handover_messages.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_handover.h"
#include "ocudu/xnap/xnap_types.h"

namespace ocudu::ocucp {

class xnap_target_handover_preparation_procedure
{
public:
  xnap_target_handover_preparation_procedure(const xnap_handover_request& request_,
                                             peer_xnap_ue_id_t            target_xnap_ue_id_,
                                             xnap_ue_context_list&        ue_ctxt_list_,
                                             xnap_cu_cp_notifier&         cu_cp_notifier_,
                                             xnap_message_notifier&       tx_notifier_,
                                             ocudulog::basic_logger&      logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "Target Handover Preparation Procedure"; }

private:
  bool create_xnap_ue(ue_index_t ue_index);

  // Result senders.
  bool send_handover_request_ack(ue_index_t                        ue_index,
                                 local_xnap_ue_id_t                local_xnap_ue_id,
                                 const cu_cp_handover_request_ack& ho_ack);
  void send_handover_preparation_failure(const cu_cp_handover_request_failure& ho_failure);

  const xnap_handover_request request;
  const peer_xnap_ue_id_t     target_xnap_ue_id;
  xnap_ue_context_list&       ue_ctxt_list;
  xnap_cu_cp_notifier&        cu_cp_notifier;
  xnap_message_notifier&      tx_notifier;
  ocudulog::basic_logger&     logger;

  // (sub-)routine results
  cu_cp_handover_resource_allocation_response response;
};

} // namespace ocudu::ocucp
