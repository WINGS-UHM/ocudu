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

namespace ocudu {
namespace ofh {

/// Open Fronthaul transmitter message processing metrics.
struct tx_data_flow_perf_metrics {
  /// Message packing statistics.
  float message_packing_min_latency_us;
  float message_packing_max_latency_us;
  float message_packing_avg_latency_us;

  /// CPU usage in microseconds of the message processing in the transmitter.
  float cpu_usage_us;
};

} // namespace ofh
} // namespace ocudu
