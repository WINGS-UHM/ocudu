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

tdd_ul_dl_config_common cell_config_builder_profiles::create_tdd_pattern(tdd_pattern_profile_fr1_30khz pattern)
{
  tdd_ul_dl_config_common cfg;
  cfg.ref_scs = subcarrier_spacing::kHz30;

  switch (pattern) {
    case tdd_pattern_profile_fr1_30khz::DDDDDDDSUU:
      // FR1.30-1.
      cfg.pattern1 = {10, 7, 6, 2, 4};
      break;
    case tdd_pattern_profile_fr1_30khz::DDDSU:
      // FR1.30-2.
      cfg.pattern1 = {5, 3, 10, 1, 2};
      break;
    case tdd_pattern_profile_fr1_30khz::DDDSUDDSUU:
      // FR1.30-3.
      cfg.pattern1 = {5, 3, 10, 1, 2};
      cfg.pattern2 = {5, 2, 10, 2, 2};
      break;
    case tdd_pattern_profile_fr1_30khz::DDDSUUDDDD:
      // FR1.30-4.
      cfg.pattern1 = {6, 3, 6, 2, 4};
      cfg.pattern2 = {4, 4, 0, 0, 0};
      break;
    case tdd_pattern_profile_fr1_30khz::DSUU:
      // FR1.30-5.
      cfg.pattern1 = {4, 1, 12, 2, 0};
      break;
    case tdd_pattern_profile_fr1_30khz::DSSU:
      // FR1.30-6.
      cfg.pattern1 = {2, 1, 10, 0, 2};
      cfg.pattern2 = {2, 0, 12, 1, 0};
      break;
    default:
      report_fatal_error("Unrecognized pattern profile");
  }
  return cfg;
}
