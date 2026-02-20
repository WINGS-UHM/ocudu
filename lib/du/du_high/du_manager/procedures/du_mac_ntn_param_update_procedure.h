// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/du/du_high/du_manager/du_configurator.h"
#include "ocudu/du/du_high/du_manager/du_manager_params.h"

namespace ocudu {
namespace odu {

class du_cell_manager;

async_task<du_ntn_param_update_response> start_du_ntn_param_update(const du_ntn_param_update_request& req,
                                                                   const du_manager_params&           params,
                                                                   du_cell_manager&                   cell_mng);

} // namespace odu
} // namespace ocudu
