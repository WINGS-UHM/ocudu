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

#include "ocudu/cu_up/o_cu_up.h"
#include "ocudu/cu_up/o_cu_up_config.h"
#include <memory>

namespace ocudu {
namespace ocuup {

/// O-RAN CU-UP interface with the given configuration and dependencies.
std::unique_ptr<o_cu_up> create_o_cu_up(const o_cu_up_config& config, o_cu_up_dependencies&& dependencies);

} // namespace ocuup
} // namespace ocudu
