// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ocudulog/ocudulog.h"
#include <string>

namespace ocudu {
namespace ocucp {

struct cu_up_processor_config_t {
  std::string                name        = "ocucp";
  cu_up_index_t              cu_up_index = cu_up_index_t::invalid;
  const cu_cp_configuration& cu_cp_cfg;
  ocudulog::basic_logger&    logger = ocudulog::fetch_basic_logger("CU-CP");
};

} // namespace ocucp
} // namespace ocudu
