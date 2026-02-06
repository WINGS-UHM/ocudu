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

#include "ocudu/ran/ntn.h"
#include <chrono>
#include <optional>
#include <variant>

namespace ocudu {

/// NTN cell-level configuration parameters.
struct ntn_cell_params {
  /// NTN cell configuration.
  ntn_config ntn_cfg;

  /// Whether UL HARQ Mode B is enabled for this NTN cell (if there is at least one UL HARQ process in mode B).
  bool ul_harq_mode_b = false;

  /// Helper method to check if NTN is enabled.
  bool is_enabled() const
  {
    return (ntn_cfg.cell_specific_koffset.has_value() and ntn_cfg.cell_specific_koffset.value().count() > 0);
  }
};

} // namespace ocudu
