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

#include "ocudu/hal/dpdk/dpdk_eal.h"

namespace ocudu {
namespace dpdk {

/// Returns a dpdk_eal instance on success, otherwise returns nullptr.
std::unique_ptr<dpdk_eal> create_dpdk_eal(const std::string& args, ocudulog::basic_logger& logger);

} // namespace dpdk
} // namespace ocudu
