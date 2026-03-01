// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/gtpu/gtpu_teid_pool.h"
#include <memory>

namespace ocudu {

struct gtpu_allocator_creation_request {
  uint32_t max_nof_teids;
};

std::unique_ptr<gtpu_teid_pool> create_gtpu_allocator(const gtpu_allocator_creation_request& msg);

} // namespace ocudu
