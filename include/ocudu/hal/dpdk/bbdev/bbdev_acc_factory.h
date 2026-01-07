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

#include "ocudu/hal/dpdk/bbdev/bbdev_acc.h"

namespace ocudu {
namespace dpdk {

/// \brief Instantiates a bbdev hardware accelerator.
/// \param[in] cfg    Configuration parameters of the bbdev-based hardware accelerator.
/// \param[in] logger OCUDU logger.
/// \return A pointer to a valid bbdev accelerator on success, otherwise \c nullptr.
std::shared_ptr<bbdev_acc> create_bbdev_acc(const bbdev_acc_configuration& cfg, ocudulog::basic_logger& logger);

} // namespace dpdk
} // namespace ocudu
