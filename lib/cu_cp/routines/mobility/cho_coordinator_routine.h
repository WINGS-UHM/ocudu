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
#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

class cu_cp_ue;
class ue_manager;
class cu_cp_impl_interface;
class du_processor_repository;

/// \brief Coordinates full intra-CU CHO flow: preparation and execution/cancellation decision.
class cho_coordinator_routine
{
public:
  cho_coordinator_routine(const cu_cp_intra_cu_cho_request& request_,
                          du_processor_repository&          du_db_,
                          cu_cp_impl_interface&             cu_cp_handler_,
                          ue_manager&                       ue_mng_,
                          ocudulog::basic_logger&           logger_);

  void operator()(coro_context<async_task<cu_cp_intra_cu_cho_response>>& ctx);

  static const char* name() { return "CHO Coordinator Routine"; }

private:
  const cu_cp_intra_cu_cho_request request;
  ue_manager&                      ue_mng;
  ocudulog::basic_logger&          logger;

  cu_cp_intra_cu_cho_response response{};

  cu_cp_ue* source_ue = nullptr;
};

} // namespace ocucp
} // namespace ocudu
