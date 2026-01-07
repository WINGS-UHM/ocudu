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

#include "ocudu/fapi/decorator.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <memory>

namespace ocudu {

class task_executor;

namespace fapi {

/// FAPI message bufferer decorator configuration.
struct message_bufferer_decorator_config {
  unsigned                  sector_id;
  unsigned                  l2_nof_slots_ahead;
  subcarrier_spacing        scs;
  task_executor&            executor;
  p7_requests_gateway&      p7_gateway;
  p7_last_request_notifier& p7_last_req_notifier;
};

/// FAPI logging decorator configuration.
struct logging_decorator_config {
  unsigned                  sector_id;
  ocudulog::basic_logger&   logger;
  p7_requests_gateway&      p7_gateway;
  p7_last_request_notifier& p7_last_req_notifier;
};

/// FAPI decorator configurations.
struct decorator_config {
  std::optional<message_bufferer_decorator_config> bufferer_cfg;
  std::optional<logging_decorator_config>          logging_cfg;
};

/// Creates and returns FAPI decorators using the given configuration or nullptr if no decorators are requested.
std::unique_ptr<fapi_decorator> create_decorators(const decorator_config& config);

} // namespace fapi
} // namespace ocudu
