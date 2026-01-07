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

#include "ocudu/du/du_low/o_du_low.h"
#include <memory>

namespace ocudu {
namespace odu {

struct o_du_low_config;
struct o_du_low_dependencies;

/// Creates and returns an O-RAN Distributed Unit (O-DU) low.
std::unique_ptr<o_du_low> make_o_du_low(const o_du_low_config& config, o_du_low_dependencies& o_du_low_deps);

} // namespace odu
} // namespace ocudu
