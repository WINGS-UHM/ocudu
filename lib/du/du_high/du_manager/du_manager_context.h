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
namespace odu {

/// General operational state of the DU.
struct du_manager_context {
  /// Whether the DU is in operational mode.
  bool running = false;
  /// Flag used to signal that the DU was commanded to stop.
  bool stop_command_received = false;
};

} // namespace odu
} // namespace ocudu
