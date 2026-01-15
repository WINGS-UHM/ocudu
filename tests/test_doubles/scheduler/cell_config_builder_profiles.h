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

#include "ocudu/ran/duplex_mode.h"
#include "ocudu/scheduler/config/cell_config_builder_params.h"

namespace ocudu {
namespace cell_config_builder_profiles {

/// Create basic cell build parameters with given duplex mode, frequency range and bandwidth.
cell_config_builder_params create(duplex_mode          mode = duplex_mode::TDD,
                                  frequency_range      fr   = frequency_range::FR1,
                                  bs_channel_bandwidth bw   = bs_channel_bandwidth::MHz20);

/// List of TDD UL-DL configurations for FR1, specified in TS 38.101-4, Table A.1.2-2.
enum class tdd_pattern_profile_fr1_30khz {
  DDDDDDDSUU, ///< FR1.30-1.
  DDDSU,      ///< FR1.30-2.
  DDDSUDDSUU, ///< FR1.30-3.
  DDDSUUDDDD, ///< FR1.30-4.
  DSUU,       ///< FR1.30-5.
  DSSU        ///< FR1.30-6.
};

tdd_ul_dl_config_common create_tdd_pattern(tdd_pattern_profile_fr1_30khz pattern);

} // namespace cell_config_builder_profiles
} // namespace ocudu
