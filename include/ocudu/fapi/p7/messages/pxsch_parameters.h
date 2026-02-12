// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include <cstdint>

namespace ocudu {
namespace fapi {

/// Common enumerations for DL_TTI.request and UL_TTI.request.
enum class low_papr_dmrs_type : uint8_t { independent_cdm_group, dependent_cdm_group };
enum class resource_allocation_type : uint8_t { type_0, type_1 };
enum class vrb_to_prb_mapping_type : uint8_t { non_interleaved, interleaved_rb_size2, interleaved_rb_size4 };

} // namespace fapi
} // namespace ocudu
