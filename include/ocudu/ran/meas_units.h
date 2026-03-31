// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/adt/bounded_integer.h"

namespace ocudu {

/// RSRP value range in dBm used in RSRP measurements and thresholds, as specified in TS 38.331, "RSRP Range" and in
/// TS 38.133, Table 10.1.6.1-1.
/// For measurements, the value -156 means "<-156", the value -155 means between [-156, -155), and so on, until -30
/// which means >=-31. Value -29 is not used for measurements.
/// For RSRP thresholds, the value -29 represents infinity.
struct rsrp_range : public strong_type<uint8_t, strong_equality, strong_comparison> {
  using base_type = strong_type<uint8_t, strong_equality, strong_comparison>;

  rsrp_range() = default;
  explicit rsrp_range(int dBm) : base_type(dBm) { ocudu_assert(dBm >= -156 and dBm <= -29, "Invalid RSRP dBm value"); }

  int16_t dBm() const { return static_cast<int16_t>(val) - 156; }

  uint8_t count() const { return val; }

  static rsrp_range infinity() { return rsrp_range{127}; }

private:
  uint8_t val{0};
};

} // namespace ocudu
