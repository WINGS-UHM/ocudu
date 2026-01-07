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

#include "ocudu/phy/upper/upper_phy_metrics.h"

namespace ocudu {
namespace odu {

/// DU low metrics.
struct du_low_metrics {
  /// Sector metrics.
  upper_phy_metrics upper_metrics;
};

} // namespace odu
} // namespace ocudu
