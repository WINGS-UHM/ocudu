// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/scheduler/config/serving_cell_config.h"

namespace ocudu {
namespace sr_helper {

/// \brief Helpers that checks if the slot is a candidate for SR reporting for a given user.
bool is_sr_opportunity_slot(const pucch_config& pucch_cfg, slot_point sl_tx);

} // namespace sr_helper
} // namespace ocudu
