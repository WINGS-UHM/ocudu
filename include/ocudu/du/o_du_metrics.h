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

#include "ocudu/du/du_high/o_du_high_metrics.h"
#include "ocudu/du/du_low/o_du_low_metrics.h"
#include <optional>

namespace ocudu {
namespace odu {

/// O-RAN DU metrics.
struct o_du_metrics {
  o_du_high_metrics               high;
  std::optional<o_du_low_metrics> low;
};

} // namespace odu
} // namespace ocudu
