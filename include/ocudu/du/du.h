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

class du_operation_controller;

namespace odu {

/// Public DU interface.
class du
{
public:
  virtual ~du() = default;

  /// Returns the operation controller of this DU.
  virtual du_operation_controller& get_operation_controller() = 0;
};

} // namespace odu
} // namespace ocudu
