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

#include "ocudu/cu_cp/cu_cp_metrics_notifier.h"
#include "ocudu/ocudulog/log_channel.h"

namespace ocudu {

/// Logger consumer for the RRC metrics.
class rrc_metrics_consumer_log
{
public:
  explicit rrc_metrics_consumer_log(ocudulog::log_channel& log_chan_) : log_chan(log_chan_)
  {
    ocudu_assert(log_chan.enabled(), "Logger log channel is not enabled");
  }

  /// Handle RRC metrics.
  void handle_metric(const std::vector<cu_cp_metrics_report::du_info>& report,
                     const mobility_management_metrics&                mobility_metrics);

private:
  ocudulog::log_channel& log_chan;
};

} // namespace ocudu
