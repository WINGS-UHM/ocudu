/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cho_coordinator_routine.h"
#include "../../ue_manager/ue_manager_impl.h"

using namespace ocudu;
using namespace ocudu::ocucp;

cho_coordinator_routine::cho_coordinator_routine(const cu_cp_intra_cu_cho_request& request_,
                                                 du_processor_repository&          du_db_,
                                                 cu_cp_impl_interface&             cu_cp_handler_,
                                                 ue_manager&                       ue_mng_,
                                                 ocudulog::basic_logger&           logger_) :
  request(request_), ue_mng(ue_mng_), logger(logger_)
{
}

void cho_coordinator_routine::operator()(coro_context<async_task<cu_cp_intra_cu_cho_response>>& ctx)
{
  CORO_BEGIN(ctx);

  if (request.source_ue_index == ue_index_t::invalid || request.source_du_index == du_index_t::invalid ||
      request.targets.empty()) {
    logger.warning("CHO coordinator request is invalid");
    CORO_EARLY_RETURN(response);
  }

  source_ue = ue_mng.find_du_ue(request.source_ue_index);
  if (source_ue == nullptr || !source_ue->get_cho_context().has_value()) {
    logger.warning("ue={}: CHO coordinator failed. Source UE/CHO context missing", request.source_ue_index);
    CORO_EARLY_RETURN(response);
  }

  // TODO: implement 3 CHO phases:
  // 1. Preparation.
  // 2. Execution.
  // 3. Completion.

  response.success = false;
  CORO_RETURN(response);
}
