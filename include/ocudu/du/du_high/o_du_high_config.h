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

#include "ocudu/du/du_high/du_high_configuration.h"
#include "ocudu/e2/e2ap_configuration.h"

namespace ocudu {

class e2_du_metrics_interface;
class e2_connection_client;

namespace fapi {
class p5_requests_gateway;
class p7_last_request_notifier;
class p7_requests_gateway;
} // namespace fapi

namespace odu {

/// FAPI configuration for the O-RAN DU high.
struct o_du_high_fapi_config {
  ocudulog::basic_levels log_level;
  unsigned               l2_nof_slots_ahead;
};

/// Base O-DU high configuration.
struct o_du_high_config {
  /// Configuration of the DU-high that comprises the MAC, RLC and F1 layers.
  odu::du_high_configuration du_hi;
  /// O-RAN DU high FAPI configuration.
  o_du_high_fapi_config fapi;
  /// E2AP configuration.
  e2ap_configuration e2ap_config;
};

/// O-RAN DU high sector dependencies. Contains the dependencies of one sector.
struct o_du_high_sector_dependencies {
  fapi::p5_requests_gateway&      p5_gateway;
  fapi::p7_requests_gateway&      p7_gateway;
  fapi::p7_last_request_notifier& p7_last_req_notifier;
  /// Timer manager.
  timer_manager& timer_mng;
  /// FAPI control executor.
  task_executor& fapi_ctrl_executor;
  /// MAC control executor.
  task_executor& mac_ctrl_executor;
  /// FAPI message bufferer executor.
  task_executor* fapi_executor = nullptr;
};

/// O-RAN DU high dependencies.
struct o_du_high_dependencies {
  std::vector<o_du_high_sector_dependencies> sectors;
  du_high_dependencies                       du_hi;
  e2_connection_client*                      e2_client          = nullptr;
  e2_du_metrics_interface*                   e2_du_metric_iface = nullptr;
};

} // namespace odu
} // namespace ocudu
