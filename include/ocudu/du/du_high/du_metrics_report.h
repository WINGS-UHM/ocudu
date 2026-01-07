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

#include "ocudu/f1ap/du/f1ap_du_metrics_collector.h"
#include "ocudu/mac/mac_metrics.h"
#include "ocudu/scheduler/scheduler_metrics.h"
#include <chrono>

namespace ocudu {
namespace odu {

/// \brief Report of the DU metrics.
struct du_metrics_report {
  unsigned                                           version = 0;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  std::chrono::milliseconds                          period;
  std::optional<f1ap_metrics_report>                 f1ap;
  std::optional<mac_metric_report>                   mac;
};

} // namespace odu
} // namespace ocudu
