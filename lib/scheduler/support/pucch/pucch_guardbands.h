// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../../config/cell_configuration.h"
#include "ocudu/ran/resource_allocation/rb_bitmap.h"

namespace ocudu {

crb_bitmap compute_pucch_crbs(const cell_configuration& cell_cfg);

} // namespace ocudu
