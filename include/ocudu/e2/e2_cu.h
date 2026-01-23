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

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/e2/e2.h"
#include "ocudu/f1u/cu_up/f1u_metrics.h"
#include "ocudu/pdcp/pdcp_metrics.h"

namespace ocudu {

class e2_cu_metrics_notifier : public pdcp_metrics_notifier, public ocuup::f1u_metrics_notifier
{
public:
  virtual ~e2_cu_metrics_notifier() = default;

  using ocuup::f1u_metrics_notifier::report_metrics;
  using pdcp_metrics_notifier::report_metrics;
};

class e2_cu_metrics_interface
{
public:
  virtual ~e2_cu_metrics_interface() = default;

  /// \brief connects e2_cu_metric_provider
  /// \param[in] meas_provider pointer to the e2_cu_metric_provider
  virtual void connect_e2_cu_meas_provider(std::unique_ptr<e2_cu_metrics_notifier> meas_provider) = 0;
};

} // namespace ocudu
