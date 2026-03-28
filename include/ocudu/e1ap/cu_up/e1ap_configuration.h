// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/support/timers.h"

namespace ocudu::ocuup {

/// Configuration for E1AP CU-UP.
struct e1ap_configuration {
  uint32_t max_nof_ues = 16384;
  /// Whether to enable JSON logging of E1AP Tx and Rx messages.
  bool           json_log_enabled = false;
  timer_duration metrics_period{0};
};

} // namespace ocudu::ocuup
