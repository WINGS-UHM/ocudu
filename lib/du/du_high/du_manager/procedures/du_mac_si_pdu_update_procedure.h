/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/du/du_high/du_manager/du_configurator.h"
#include "ocudu/du/du_high/du_manager/du_manager_params.h"

namespace ocudu {
namespace odu {

class du_cell_manager;

async_task<du_si_pdu_update_response> start_du_mac_si_pdu_update(const du_si_pdu_update_request& req,
                                                                 const du_manager_params&        params,
                                                                 du_cell_manager&                cell_mng);

} // namespace odu
} // namespace ocudu
