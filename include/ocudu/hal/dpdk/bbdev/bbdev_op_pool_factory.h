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

#include "ocudu/hal/dpdk/bbdev/bbdev_op_pool.h"
#include "ocudu/ocudulog/logger.h"
#include <rte_bbdev_op.h>

namespace ocudu {
namespace dpdk {

/// Returns a bbdev_op_pool instance on success, otherwise returns nullptr.
std::unique_ptr<bbdev_op_pool> create_bbdev_op_pool(const char*             pool_name,
                                                    ::rte_bbdev_op_type     op_type,
                                                    uint16_t                nof_elements,
                                                    int                     socket,
                                                    ocudulog::basic_logger& logger);

} // namespace dpdk
} // namespace ocudu
