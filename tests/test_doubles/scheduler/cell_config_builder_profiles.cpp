/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cell_config_builder_profiles.h"
#include "ocudu/ran/band_helper.h"

using namespace ocudu;

/// Create cell build parameters for a TDD band.
static cell_config_builder_params tdd(bs_channel_bandwidth bw)
{
  cell_config_builder_params params{};
  params.scs_common             = subcarrier_spacing::kHz30;
  params.dl_carrier.arfcn_f_ref = 520002;
  params.dl_carrier.band        = band_helper::get_band_from_dl_arfcn(params.dl_carrier.arfcn_f_ref);
  params.dl_carrier.carrier_bw  = bw;
  return params;
}

/// Create cell build parameters for a FDD band.
static cell_config_builder_params fdd(bs_channel_bandwidth bw)
{
  cell_config_builder_params params{};
  params.scs_common             = subcarrier_spacing::kHz15;
  params.dl_carrier.arfcn_f_ref = 530000;
  params.dl_carrier.band        = band_helper::get_band_from_dl_arfcn(params.dl_carrier.arfcn_f_ref);
  params.dl_carrier.carrier_bw  = bw;
  return params;
}

/// Create cell build parameters for a TDD FR2 band.
static cell_config_builder_params tdd_fr2(bs_channel_bandwidth bw)
{
  cell_config_builder_params params{};
  params.scs_common             = subcarrier_spacing::kHz120;
  params.dl_carrier.arfcn_f_ref = 2074171;
  params.dl_carrier.band        = band_helper::get_band_from_dl_arfcn(params.dl_carrier.arfcn_f_ref);
  params.dl_carrier.carrier_bw  = bw;
  return params;
}

cell_config_builder_params
cell_config_builder_profiles::create(duplex_mode mode, frequency_range fr, bs_channel_bandwidth bw)
{
  if (mode == duplex_mode::TDD) {
    if (fr == frequency_range::FR1) {
      return tdd(bw);
    }
    return tdd_fr2(bw);
  }
  report_error_if_not(fr == frequency_range::FR1, "FDD bands are only supported in FR1");
  return fdd(bw);
}
