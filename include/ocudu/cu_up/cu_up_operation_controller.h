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

namespace ocudu {
namespace ocuup {

/// CU-UP operation controller interface that allows to start/stop the CU-UP.
class cu_up_operation_controller
{
public:
  virtual ~cu_up_operation_controller() = default;

  /// Starts the CU-UP.
  virtual void start() = 0;

  /// Stops the CU-UP.
  virtual void stop() = 0;
};

} // namespace ocuup
} // namespace ocudu
