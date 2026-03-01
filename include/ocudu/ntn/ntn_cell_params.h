// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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
