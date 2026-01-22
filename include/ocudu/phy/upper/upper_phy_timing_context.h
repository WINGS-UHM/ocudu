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

#include "ocudu/ran/slot_point_extended.h"
#include <chrono>

namespace ocudu {

/// Describes the context of the current timing boundary.
struct upper_phy_timing_context {
  /// Indicates the current slot.
  slot_point_extended slot;
  /// Indicates the system time point associated to the current slot.
  std::chrono::time_point<std::chrono::system_clock> time_point;
};

} // namespace ocudu
