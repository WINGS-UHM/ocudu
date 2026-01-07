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

#include "ocudu/du/du_high/du_manager/du_manager.h"
#include "ocudu/du/du_high/du_manager/du_manager_params.h"

namespace ocudu {
namespace odu {

/// Creates an instance of a DU manager.
std::unique_ptr<du_manager_interface> create_du_manager(const du_manager_params& params);

} // namespace odu
} // namespace ocudu
