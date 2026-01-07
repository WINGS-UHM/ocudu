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

#include "ocudu/ofh/ofh_metrics.h"
#include "ocudu/ru/dummy/ru_dummy_metrics.h"
#include "ocudu/ru/sdr/ru_sdr_metrics.h"
#include <variant>

namespace ocudu {

/// Radio Unit metrics.
struct ru_metrics {
  std::variant<ru_dummy_metrics, ru_sdr_metrics, ofh::metrics> metrics;
};

} // namespace ocudu
