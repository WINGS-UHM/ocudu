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

#include "ocudu/du/du_high/du_test_mode_config.h"
#include "ocudu/mac/mac.h"
#include "ocudu/mac/mac_config.h"

namespace ocudu {
namespace odu {

/// \brief Create a MAC instance for DU-high. In case the test mode is enabled, the MAC messages will be intercepted.
std::unique_ptr<mac_interface>
create_du_high_mac(const mac_config& mac_cfg, const odu::du_test_mode_config& test_cfg, unsigned nof_cells);

} // namespace odu
} // namespace ocudu
