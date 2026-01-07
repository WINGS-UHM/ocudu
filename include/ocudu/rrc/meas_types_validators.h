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

#include "ocudu/rrc/meas_types.h"

namespace ocudu {
namespace ocucp {

/// Validates the provided config(s).
bool validate_config(const rrc_meas_trigger_quant_offset& config);
bool validate_config(const rrc_cond_event_a3& config);

} // namespace ocucp
} // namespace ocudu
