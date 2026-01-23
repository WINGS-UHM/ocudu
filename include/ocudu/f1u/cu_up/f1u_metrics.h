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
namespace ocuup {

struct f1u_metrics_container;

/// \brief Notifier interface used to report F1-U metrics.
class f1u_metrics_notifier
{
public:
  virtual ~f1u_metrics_notifier() = default;

  /// \brief This method will be called periodically to report the latest F1-U metrics statistics.
  virtual void report_metrics(const f1u_metrics_container& metrics) = 0;
};
} // namespace ocuup
} // namespace ocudu
