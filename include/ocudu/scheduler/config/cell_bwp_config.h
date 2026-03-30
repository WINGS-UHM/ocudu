// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/scheduler/config/ran_cell_config.h"
#include <vector>

namespace ocudu {

struct cell_pucch_config {
  std::vector<pucch_resource> resources;

  bool operator==(const cell_pucch_config& other) const { return resources == other.resources; }
};

/// \brief Cell-common information of an UL BWP configuration.
///
/// This structure contains all the UL BWP parameters that are common across all UEs.
struct cell_uplink_bwp_config {
  cell_pucch_config pucch;

  bool operator==(const cell_uplink_bwp_config& other) const { return pucch == other.pucch; }
};

struct cell_bwp_config {
  cell_uplink_bwp_config ul;

  bool operator==(const cell_bwp_config& other) const { return ul == other.ul; }
};

// Generate the cell-common BWP configuration from the cell configuration.
cell_bwp_config make_cell_bwp_config(const ran_cell_config& cell_cfg);

} // namespace ocudu
