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

#include <string>

namespace ocudu {
namespace app_services {

/// Remote server metrics gateway.
class remote_server_metrics_gateway
{
public:
  virtual ~remote_server_metrics_gateway() = default;

  /// Sends the given metrics through the gateway.
  virtual void send(std::string metrics) = 0;
};

} // namespace app_services
} // namespace ocudu
