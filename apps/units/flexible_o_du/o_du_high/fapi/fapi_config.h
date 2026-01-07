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

#include "ocudu/ocudulog/logger.h"

namespace ocudu {

/// FAPI configuration.
struct fapi_unit_config {
  /// Number of slots the L2 is running ahead of the L1.
  unsigned l2_nof_slots_ahead = 0;
  /// FAPI log level.
  ocudulog::basic_levels fapi_level = ocudulog::basic_levels::warning;
};

} // namespace ocudu
