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

#include <cstdint>
#include <optional>
#include <utility>

namespace ocudu {

/// \brief Clipping event counters.
///
/// It comprises of the number of clipped samples and the total number of processed samples.
struct clipping_counters {
  uint64_t nof_clipped_samples;
  uint64_t nof_processed_samples;

  bool operator==(const clipping_counters& other) const
  {
    return (nof_clipped_samples == other.nof_clipped_samples) && (nof_processed_samples == other.nof_processed_samples);
  }
  bool operator!=(const clipping_counters& other) const { return !operator==(other); }
};

/// Collects transmit or receive signal statistics.
struct lower_phy_baseband_metrics {
  /// Average power.
  float avg_power;
  /// Peak power.
  float peak_power;
  /// Clipping counters.
  std::optional<clipping_counters> clipping;
};

} // namespace ocudu
