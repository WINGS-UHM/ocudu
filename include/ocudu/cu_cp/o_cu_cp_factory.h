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

#include "ocudu/cu_cp/o_cu_cp.h"
#include <memory>

namespace ocudu {
namespace ocucp {

struct o_cu_cp_config;
struct o_cu_cp_dependencies;

std::unique_ptr<o_cu_cp> create_o_cu_cp(const o_cu_cp_config& config, const o_cu_cp_dependencies& dependencies);

} // namespace ocucp
} // namespace ocudu
