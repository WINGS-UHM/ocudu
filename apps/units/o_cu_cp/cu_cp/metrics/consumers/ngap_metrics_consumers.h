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

#include "ocudu/cu_cp/mobility_management_metrics.h"
#include "ocudu/ngap/ngap_metrics.h"
#include "ocudu/ocudulog/log_channel.h"
#include <optional>

namespace ocudu {

/// Logger consumer for the NGAP metrics.
class ngap_metrics_consumer_log
{
public:
  explicit ngap_metrics_consumer_log(ocudulog::log_channel& log_chan_) : log_chan(log_chan_)
  {
    ocudu_assert(log_chan.enabled(), "Logger log channel is not enabled");
  }

  /// Handle NGAP metrics.
  void handle_metric(const std::vector<ngap_info>& report, const mobility_management_metrics& mobility_metrics);

private:
  ocudulog::log_channel& log_chan;
};

} // namespace ocudu
