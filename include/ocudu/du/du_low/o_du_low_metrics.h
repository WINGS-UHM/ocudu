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

#include "ocudu/du/du_low/du_low_metrics.h"

namespace ocudu {
namespace odu {

/// O-RAN DU low metrics.
struct o_du_low_metrics {
  /// DU low metrics.
  du_low_metrics du_lo_metrics;
};

} // namespace odu
} // namespace ocudu
