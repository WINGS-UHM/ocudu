// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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
