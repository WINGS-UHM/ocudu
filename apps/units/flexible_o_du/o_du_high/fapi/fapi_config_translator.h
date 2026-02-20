// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

namespace ocudu {

struct fapi_unit_config;
struct worker_manager_config;

/// Fills the FAPI worker manager parameters of the given worker manager configuration.
void fill_fapi_worker_manager_config(worker_manager_config&  config,
                                     const fapi_unit_config& unit_cfg,
                                     unsigned                nof_cells);

} // namespace ocudu
