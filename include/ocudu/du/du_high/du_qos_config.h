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

#include "ocudu/f1u/du/f1u_config.h"
#include "ocudu/rlc/rlc_config.h"

namespace ocudu {
namespace odu {

/// \brief QoS Configuration, i.e. 5QI and the associated RLC configuration for DRBs
struct du_qos_config {
  rlc_config rlc;
  f1u_config f1u;
};

} // namespace odu
} // namespace ocudu
