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

#include "ocudu/cu_cp/cu_cp_cho_types.h"
#include "ocudu/cu_cp/cu_cp_intra_cu_ho_types.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {
class mobility_manager;

class cu_cp_ue;
class ue_manager;
class cu_cp_impl_interface;
class du_processor_repository;

/// \brief Tracks prepared CHO targets.
/// Used to release orphaned targets if the source UE disappears before CHO completes.
struct prepared_cho_target {
  ue_index_t target_ue_index;
  du_index_t target_du_index;
};

/// \brief Coordinates full intra-CU CHO flow: preparation and execution/cancellation decision.
class cho_coordinator_routine
{
public:
  cho_coordinator_routine(const cu_cp_intra_cu_cho_request& request_,
                          du_processor_repository&          du_db_,
                          cu_cp_impl_interface&             cu_cp_handler_,
                          ue_manager&                       ue_mng_,
                          mobility_manager&                 mobility_mng_,
                          ocudulog::basic_logger&           logger_);

  void operator()(coro_context<async_task<cu_cp_intra_cu_cho_response>>& ctx);

  static const char* name() { return "CHO Coordinator Routine"; }

private:
  const cu_cp_intra_cu_cho_request request;
  du_processor_repository&         du_db;
  cu_cp_impl_interface&            cu_cp_handler;
  ue_manager&                      ue_mng;
  mobility_manager&                mobility_mng; // needed for intra_cu_handover_routine
  ocudulog::basic_logger&          logger;

  cu_cp_intra_cu_cho_response response{};

  cu_cp_ue*                        source_ue = nullptr;
  byte_buffer                      target_cell_sib1;
  du_index_t                       source_du_index = du_index_t::invalid;
  du_index_t                       target_du_index = du_index_t::invalid;
  size_t                           candidate_idx   = 0;
  cond_recfg_id_t                  cond_recfg_id   = 1;
  cu_cp_intra_cu_handover_request  prep_request;
  cu_cp_intra_cu_handover_response prep_response;

  /// \brief Releases all tracked inter-DU target UE contexts.
  /// Called when the source UE disappears before CHO completes.
  async_task<void>                 release_prepared_targets();
  std::vector<prepared_cho_target> prepared_cho_targets;
};

} // namespace ocucp
} // namespace ocudu
