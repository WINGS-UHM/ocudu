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

#include "ue_context/ngap_ue_context.h"
#include "ocudu/ngap/ngap.h"
#include "ocudu/ngap/ngap_handover.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

class ngap_handover_resource_allocation_procedure
{
public:
  ngap_handover_resource_allocation_procedure(const ngap_handover_request& request_,
                                              const amf_ue_id_t            amf_ue_id_,
                                              ngap_ue_context_list&        ue_ctxt_list_,
                                              ngap_cu_cp_notifier&         cu_cp_notifier_,
                                              ngap_message_notifier&       amf_notifier_,
                                              ocudulog::basic_logger&      logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "Handover Resource Allocation Procedure"; }

private:
  bool create_ngap_ue(ue_index_t ue_index);

  // Result senders.
  bool send_handover_request_ack(ue_index_t ue_index, ran_ue_id_t ran_ue_id);
  void send_handover_failure();

  const ngap_handover_request request;
  const amf_ue_id_t           amf_ue_id;
  ngap_ue_context_list&       ue_ctxt_list;
  ngap_cu_cp_notifier&        cu_cp_notifier;
  ngap_message_notifier&      amf_notifier;
  ocudulog::basic_logger&     logger;

  // (sub-)routine results
  ngap_handover_resource_allocation_response response;
};

} // namespace ocudu::ocucp
