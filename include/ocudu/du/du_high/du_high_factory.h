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

#include "ocudu/du/du_high/du_high.h"
#include "ocudu/du/du_high/du_high_configuration.h"

namespace ocudu {
namespace odu {

/// Create a DU-high instance, which comprises MAC, RLC and F1 layers.
std::unique_ptr<du_high> make_du_high(const du_high_configuration& du_hi_cfg, const du_high_dependencies& dependencies);

} // namespace odu
} // namespace ocudu
