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

#include "ocudu/support/async/async_task.h"

namespace ocudu {

/// PHY cell operation controller that handles cell activation/deactivation.
class phy_cell_operation_controller
{
public:
  virtual ~phy_cell_operation_controller() = default;

  /// Starts the cell.
  virtual async_task<bool> start() = 0;

  /// Stops the cell.
  virtual async_task<bool> stop() = 0;
};

} // namespace ocudu
