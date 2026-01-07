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

#include "ocudu/du/o_du.h"
#include "ocudu/du/o_du_config.h"
#include <memory>

namespace ocudu {
namespace odu {

/// Instantiates an O-RAN Distributed Unit (O-DU) object with the given configuration and dependencies.
std::unique_ptr<o_du> make_o_du(o_du_dependencies&& dependencies);

} // namespace odu
} // namespace ocudu
