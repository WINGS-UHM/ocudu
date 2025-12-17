/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/scheduler/config/cell_config_builder_params.h"

namespace ocudu {
namespace cell_config_builder_profiles {

/// Create cell build parameters for a TDD band.
cell_config_builder_params tdd(bs_channel_bandwidth bw = bs_channel_bandwidth::MHz20);

/// Create cell build parameters for a FDD band.
cell_config_builder_params fdd(bs_channel_bandwidth bw = bs_channel_bandwidth::MHz20);

/// Create cell build parameters for a TDD FR2 band.
cell_config_builder_params tdd_fr2(bs_channel_bandwidth bw = bs_channel_bandwidth::MHz100);

} // namespace cell_config_builder_profiles
} // namespace ocudu
