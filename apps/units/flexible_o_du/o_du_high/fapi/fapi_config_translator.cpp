// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "fapi_config_translator.h"
#include "apps/services/worker_manager/worker_manager_config.h"
#include "fapi_config.h"

using namespace ocudu;

void ocudu::fill_fapi_worker_manager_config(worker_manager_config&  config,
                                            const fapi_unit_config& unit_cfg,
                                            unsigned                nof_cells)
{
  // No configuration for FAPI if there is no buffered module.
  if (unit_cfg.l2_nof_slots_ahead == 0) {
    return;
  }

  auto& fapi_cfg     = config.fapi_cfg.emplace();
  fapi_cfg.nof_cells = nof_cells;
}
