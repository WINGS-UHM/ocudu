// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/ran/slot_point_extended.h"
#include <chrono>

namespace ocudu {
namespace fapi {

/// Slot indication message.
struct slot_indication {
  slot_point_extended slot;
  /// Vendor specific properties.
  std::chrono::time_point<std::chrono::system_clock> time_point;
};

} // namespace fapi
} // namespace ocudu
