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

#include <cstdint>
#include <optional>

namespace ocudu::ocuup {

/// \brief Public interface to the main CU-UP class
class cu_up_interface
{
public:
  virtual ~cu_up_interface() = default;

  virtual void start() = 0;

  /// \brief Stop the CU-UP. This call is blocking and only returns once all tasks in the CU-UP are completed.
  virtual void stop() = 0;
};

} // namespace ocudu::ocuup
