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
namespace odu {

struct o_du_high_metrics;

/// O-RAN DU high metrics notifier.
class o_du_high_metrics_notifier
{
public:
  virtual ~o_du_high_metrics_notifier() = default;

  /// Notifies new O-DU high metrics.
  virtual void on_new_metrics(const o_du_high_metrics& metrics) = 0;
};

} // namespace odu
} // namespace ocudu
