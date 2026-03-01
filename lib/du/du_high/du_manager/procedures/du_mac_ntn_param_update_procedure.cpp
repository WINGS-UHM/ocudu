// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "du_mac_ntn_param_update_procedure.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/async/async_no_op_task.h"
#include <future>

using namespace ocudu;
using namespace odu;

#ifndef OCUDU_HAS_ENTERPRISE_NTN

async_task<du_ntn_param_update_response> ocudu::odu::start_du_ntn_param_update(const du_ntn_param_update_request& req,
                                                                               const du_manager_params& params,
                                                                               du_cell_manager&         cell_mng)
{
  auto err_function = [](coro_context<async_task<du_ntn_param_update_response>>& ctx) {
    CORO_BEGIN(ctx);
    CORO_RETURN(du_ntn_param_update_response{false});
  };
  return launch_async(std::move(err_function));
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
