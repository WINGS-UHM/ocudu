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

#include "cu_up_f1u_metrics.h"
#include "ocudu/ocudulog/log_channel.h"

namespace ocudu {

namespace ocuup {
class f1u_metrics_notifier;
}

/// Consumer for the E2 CU-UP F1-U metrics.
class cu_up_f1u_metrics_consumer_e2 : public app_services::metrics_consumer
{
public:
  explicit cu_up_f1u_metrics_consumer_e2(ocuup::f1u_metrics_notifier& notifier_) : notifier(notifier_) {}

  // See interface for documentation.
  void handle_metric(const app_services::metrics_set& metric) override;

private:
  ocuup::f1u_metrics_notifier& notifier;
};

/// Consumer for the json CU-UP F1-U metrics.
class cu_up_f1u_metrics_consumer_json : public app_services::metrics_consumer
{
  struct aggregated_metrics {
    void reset()
    {
      tx             = {};
      rx             = {};
      metrics_period = {};
      is_empty       = true;
    }

    bool is_empty = true;

    ocuup::f1u_tx_metrics_container tx;
    ocuup::f1u_rx_metrics_container rx;
    timer_duration                  metrics_period;
  };

public:
  cu_up_f1u_metrics_consumer_json(ocudulog::basic_logger& logger_,
                                  ocudulog::log_channel&  log_chan_,
                                  task_executor&          executor_,
                                  unique_timer            timer_,
                                  unsigned                report_period_ms_);

  // See interface for documentation.
  void handle_metric(const app_services::metrics_set& metric) override;

private:
  // Print metrics.
  void print_metrics();

  // Clear metrics.
  void clear_metrics() { aggr_metrics.reset(); }

  // Initialize timer.
  void initialize_timer();

  const unsigned          report_period_ms;
  ocudulog::basic_logger& logger;
  ocudulog::log_channel&  log_chan;
  task_executor&          executor;
  unique_timer            timer;
  aggregated_metrics      aggr_metrics;
};

/// Consumer for the log F1-U metrics.
class cu_up_f1u_metrics_consumer_log : public app_services::metrics_consumer
{
public:
  explicit cu_up_f1u_metrics_consumer_log(ocudulog::log_channel& log_chan_) : log_chan(log_chan_)
  {
    ocudu_assert(log_chan.enabled(), "Logger log channel is not enabled");
  }

  // See interface for documentation.
  void handle_metric(const app_services::metrics_set& metric) override;

private:
  ocudulog::log_channel& log_chan;
};

} // namespace ocudu
