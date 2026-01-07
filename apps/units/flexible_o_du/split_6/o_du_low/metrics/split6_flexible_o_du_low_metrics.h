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

#include "ocudu/du/du_low/o_du_low_metrics.h"
#include "ocudu/ru/ru_metrics.h"

namespace ocudu {

/// Flexible O-RAN DU metrics.
struct split6_flexible_o_du_low_metrics {
  odu::o_du_low_metrics du_low;
  ru_metrics            ru;
};

} // namespace ocudu
