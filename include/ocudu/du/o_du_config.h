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

#include "ocudu/du/du_high/o_du_high.h"
#include "ocudu/du/du_low/o_du_low.h"
#include <memory>

namespace ocudu {
namespace odu {

class o_du_metrics_notifier;

/// O-RAN Distributed Unit dependencies.
struct o_du_dependencies {
  o_du_metrics_notifier*     metrics_notifier = nullptr;
  std::unique_ptr<o_du_high> odu_hi;
  std::unique_ptr<o_du_low>  odu_lo;
};

} // namespace odu
} // namespace ocudu
