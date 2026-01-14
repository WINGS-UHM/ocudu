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

#include "ocudu/mac/mac.h"
#include "ocudu/mac/mac_config.h"
#include <memory>

namespace ocudu {

std::unique_ptr<mac_interface> create_mac(const mac_config& mac_cfg);

} // namespace ocudu
