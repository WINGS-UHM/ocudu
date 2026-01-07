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

#include "ocudu/rlc/rlc_config.h"

namespace ocudu {
namespace odu {

/// \brief SRB Configuration, i.e. associated RLC and MAC configuration for SRBs in the DU
struct du_srb_config {
  rlc_config rlc;
};

} // namespace odu
} // namespace ocudu
