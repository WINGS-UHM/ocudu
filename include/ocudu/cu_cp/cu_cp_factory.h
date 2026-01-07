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

#include "ocudu/cu_cp/cu_cp.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include <memory>

namespace ocudu {

/// Creates an instance of an CU-CP.
std::unique_ptr<ocucp::cu_cp> create_cu_cp(const ocucp::cu_cp_configuration& cfg_);

} // namespace ocudu
