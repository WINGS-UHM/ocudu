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

#include "../ue_manager/ue_manager_impl.h"
#include "ocudu/f1ap/cu_cp/f1ap_cu_configuration_update.h"
#include <unordered_set>

namespace ocudu {
namespace ocucp {

/// \brief Handles the release of the connected UEs and the deactivation of the cell.
class cell_deactivation_routine
{
public:
  cell_deactivation_routine(const cu_cp_configuration&        cu_cp_cfg_,
                            const std::vector<plmn_identity>& plmns_,
                            du_processor_repository&          du_db_,
                            cu_cp_ue_context_release_handler& ue_release_handler_,
                            ue_manager&                       ue_mng_,
                            ocudulog::basic_logger&           logger_);
  ~cell_deactivation_routine() = default;

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "Cell Deactivation Routine"; }

  void release_ues();

  void get_remaining_plmns(const du_cell_configuration& cell_cfg);

  bool generate_gnb_cu_configuration_update();

private:
  const cu_cp_configuration&        cu_cp_cfg;
  const std::vector<plmn_identity>  plmns;
  du_processor_repository&          du_db;
  cu_cp_ue_context_release_handler& ue_release_handler;
  ue_manager&                       ue_mng;
  ocudulog::basic_logger&           logger;

  unique_timer ue_release_timer;

  // (Sub-)Routine requests.
  f1ap_gnb_cu_configuration_update f1ap_cu_cfg_update;

  // (Sub-)Routine results.
  f1ap_gnb_cu_configuration_update_response f1ap_cu_cfg_update_response;
  bool                                      routine_success = true;

  std::unordered_map<ue_index_t, bool>           ue_release_status;
  std::unordered_map<ue_index_t, bool>::iterator ue_release_status_it;
  bool                                           all_ues_released = false;
  std::unordered_set<plmn_identity>              remaining_plmns;
  std::vector<du_index_t>                        du_indexes;
  std::vector<du_index_t>::iterator              du_idx_it;
  du_processor*                                  du_proc = nullptr;
};

} // namespace ocucp
} // namespace ocudu
