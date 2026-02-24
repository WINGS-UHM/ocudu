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

#include "../../cu_cp_impl_interface.h"
#include "../../ue_manager/ue_manager_impl.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

class du_processor_repository;
class cu_up_processor_repository;
class mobility_manager;

/// \brief Finalizes CHO completion on source side upon ACCESS SUCCESS.
///
/// This routine resolves the source UE CHO context, determines winner candidate,
/// performs inter-DU completion actions (context transfer, CU-UP update, target DU
/// confirmation), releases non-winners, and clears source CHO context.
class cho_source_routine
{
public:
  cho_source_routine(const cu_cp_access_success_indication& msg_,
                     ue_manager&                            ue_mng_,
                     du_processor_repository&               du_db_,
                     cu_up_processor_repository&            cu_up_db_,
                     cu_cp_ue_context_manipulation_handler& cu_cp_handler_,
                     cu_cp_ue_context_release_handler&      ue_context_release_handler_,
                     mobility_manager&                      mobility_mng_,
                     ocudulog::basic_logger&                logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "CHO Source Routine"; }

private:
  bool fill_bearer_context_security_info(e1ap_bearer_context_modification_request& request,
                                         const security::sec_as_config&            sec_cfg);
  bool resolve_source_ue();

  const cu_cp_access_success_indication msg;

  ue_manager&                            ue_mng;
  du_processor_repository&               du_db;
  cu_up_processor_repository&            cu_up_db;
  cu_cp_ue_context_manipulation_handler& cu_cp_handler;
  cu_cp_ue_context_release_handler&      ue_context_release_handler;
  mobility_manager&                      mobility_mng;
  ocudulog::basic_logger&                logger;

  cu_cp_ue*  source_ue       = nullptr;
  cu_cp_ue*  target_ue       = nullptr;
  ue_index_t source_ue_index = ue_index_t::invalid;

  cu_cp_cho_candidate* winner             = nullptr;
  bool                 is_inter_du_winner = false;

  std::vector<ue_index_t> inter_du_targets_to_release;
  size_t                  release_idx = 0;

  cu_cp_ue_context_release_command  release_cmd;
  cu_cp_ue_context_release_complete release_complete;

  e1ap_bearer_context_modification_request  bearer_ctx_mod_request;
  e1ap_bearer_context_modification_response bearer_ctx_mod_response;
  f1ap_ue_context_modification_request      target_du_context_mod_request;
};

} // namespace ocucp
} // namespace ocudu
