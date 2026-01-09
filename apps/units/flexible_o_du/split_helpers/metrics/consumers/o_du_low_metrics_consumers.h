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

#include "ocudu/adt/span.h"

namespace ocudu {

namespace app_services {
class remote_server_metrics_gateway;
} // namespace app_services

namespace odu {
struct o_du_low_metrics;
} // namespace odu

/// JSON handler for the O-DU low metrics.
class o_du_low_metrics_consumer_json
{
public:
  explicit o_du_low_metrics_consumer_json(app_services::remote_server_metrics_gateway& gateway_) : gateway(gateway_) {}

  // Handles the O-RU metrics.
  void handle_metric(const odu::o_du_low_metrics& metric);

private:
  app_services::remote_server_metrics_gateway& gateway;
};

/// Logger consumer for the O-DU low metrics.
class o_du_low_metrics_consumer_log
{
public:
  o_du_low_metrics_consumer_log(ocudulog::log_channel& log_chan_, bool verbose_) :
    log_chan(log_chan_), verbose(verbose_)
  {
    ocudu_assert(log_chan.enabled(), "JSON log channel is not enabled");
  }

  // Handles the O-RU metrics.
  void handle_metric(const odu::o_du_low_metrics& metric);

private:
  ocudulog::log_channel& log_chan;
  const bool             verbose;
};

} // namespace ocudu
