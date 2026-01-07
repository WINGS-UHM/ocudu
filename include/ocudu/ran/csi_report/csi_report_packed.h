/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/support/units.h"

namespace ocudu {

/// \brief Maximum Channel State Information (CSI) report size in bits.
///
/// Maximum CSI-Part1 report payload size for report quantity \e cri-RI-LI-PMI-CQI, wideband, four port
/// TypeI-SinglePanel PMI codebook, and multiplexed PUCCH.
constexpr units::bits csi_report_max_size(17U);

/// Packed Channel State Information (CSI) report data type.
using csi_report_packed = bounded_bitset<csi_report_max_size.value(), false>;

} // namespace ocudu
