// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include <cstdint>

namespace ocudu {

/// \brief Single-panel PMI Codebook antenna panel configurations.
///
/// The enumeration contains the possible combinations of \f$N_1, N_2\f$ given in TS38.214 Section 5.2.2.2.1.
///
/// The value of the enumeration is mapped to each of the rows in TS38.214 Table 5.2.2.2.1-2, where the product of
/// \f$N_1\times N_2\f$ is equal to half the number of CSI-RS antenna ports \f$P_{CSI-RS}\f$.
enum class pmi_codebook_single_panel_config : uint8_t {
  two_one     = 0,
  two_two     = 1,
  four_one    = 2,
  three_two   = 3,
  six_one     = 4,
  four_two    = 5,
  eight_one   = 6,
  four_three  = 7,
  six_two     = 8,
  twelve_one  = 9,
  four_four   = 10,
  eight_two   = 11,
  sixteen_one = 12
};

/// \brief PMI Codebook Type I mode enumeration.
///
/// This parameter is given by the higher layer parameter \e codebookMode in the Information Element \e CodebookConfig.
enum class pmi_codebook_typeI_mode : uint8_t { one = 1, two = 2 };

} // namespace ocudu
