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

#include "ocudu/f1ap/du/f1ap_du_metrics_report.h"

namespace ocudu {
namespace odu {

/// \brief Class used to collect F1AP metrics.
class f1ap_metrics_collector
{
public:
  virtual ~f1ap_metrics_collector() = default;

  /// \brief Generate an F1AP metrics report.
  /// \param report Report to fill.
  virtual void collect_metrics_report(f1ap_metrics_report& report) = 0;
};

} // namespace odu
} // namespace ocudu
