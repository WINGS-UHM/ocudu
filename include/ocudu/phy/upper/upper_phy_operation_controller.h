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

/// Operation controller to start/stop an upper PHY instance.
class upper_phy_operation_controller
{
public:
  virtual ~upper_phy_operation_controller() = default;

  /// Starts the upper PHY.
  virtual void start() = 0;

  /// Stops the upper PHY.
  virtual void stop() = 0;
};

} // namespace ocudu
