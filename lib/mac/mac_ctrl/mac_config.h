/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/du/du_high/du_high_executor_mapper.h"
#include "ocudu/mac/mac.h"
#include "ocudu/ocudulog/ocudulog.h"

namespace ocudu {

class mac_metrics_notifier;
class timer_manager;
class mac_clock_controller;

/// Config for MAC controller.
struct mac_control_config {
  struct metrics_config {
    std::chrono::milliseconds period{1000};
    mac_metrics_notifier&     mac_notifier;
    unsigned                  max_nof_ue_events;
  };

  mac_ul_ccch_notifier&   event_notifier;
  task_executor&          ctrl_exec;
  mac_clock_controller&   time_source;
  metrics_config          metrics;
  ocudulog::basic_logger& logger = ocudulog::fetch_basic_logger("MAC", true);
};

} // namespace ocudu
