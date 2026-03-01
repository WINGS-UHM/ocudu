// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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
