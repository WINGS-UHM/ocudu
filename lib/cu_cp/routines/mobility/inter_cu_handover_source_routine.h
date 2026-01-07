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

#include "../../cu_up_processor/cu_up_processor_repository.h"
#include "../../du_processor/du_processor_repository.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

class inter_cu_handover_source_routine
{
public:
  inter_cu_handover_source_routine(ue_index_t                    ue_index_,
                                   byte_buffer                   command,
                                   ue_manager&                   ue_mng_,
                                   du_processor_repository&      du_db_,
                                   cu_up_processor_repository&   cu_up_db_,
                                   ngap_control_message_handler& ngap_,
                                   ocudulog::basic_logger&       logger_);

  void operator()(coro_context<async_task<bool>>& ctx);

  static const char* name() { return "Inter CU Handover Source Routine"; }

private:
  void fill_e1ap_bearer_modification_request_pdcp_sn_query();
  bool fill_ngap_ul_ran_status_transfer();

  ue_index_t                    ue_index;
  byte_buffer                   command;
  ue_manager&                   ue_mng;
  du_processor_repository&      du_db;
  cu_up_processor_repository&   cu_up_db;
  ngap_control_message_handler& ngap;

  ocudulog::basic_logger& logger;

  byte_buffer                               ho_reconfig_pdu;
  e1ap_bearer_context_modification_request  bearer_mod_req          = {};
  e1ap_bearer_context_modification_response bearer_mod_resp         = {};
  f1ap_ue_context_modification_response     ue_context_mod_response = {};
  f1ap_ue_context_modification_request      ue_context_mod_request  = {};
  ngap_ul_ran_status_transfer               ul_ran_status_transfer  = {};
};

} // namespace ocudu::ocucp
