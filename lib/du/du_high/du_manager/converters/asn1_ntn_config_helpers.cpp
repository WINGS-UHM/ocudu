/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "asn1_ntn_config_helpers.h"

using namespace ocudu;
using namespace asn1::rrc_nr;

/// Pack NTN configuration parameters into ASN.1 NTN-Config structure.
static ntn_cfg_r17_s make_asn1_rrc_cell_ntn_cfg(const sib19_info& sib19_params)
{
  ntn_cfg_r17_s ntn_cfg;

  // Epoch Time. Exempt from SI change notification or valueTag modification in SIB1.
  if (sib19_params.epoch_time.has_value()) {
    ntn_cfg.epoch_time_r17_present          = true;
    ntn_cfg.epoch_time_r17.sfn_r17          = sib19_params.epoch_time.value().sfn;
    ntn_cfg.epoch_time_r17.sub_frame_nr_r17 = sib19_params.epoch_time.value().subframe_number;
  }

  // Validity duration for UL sync assistance info in seconds.
  // Exempt from SI change notification or valueTag modification in SIB1.
  if (sib19_params.ntn_ul_sync_validity_dur.has_value()) {
    ntn_cfg.ntn_ul_sync_validity_dur_r17_present = true;
    switch (sib19_params.ntn_ul_sync_validity_dur.value()) {
      case 5:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s5;
        break;
      case 10:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s10;
        break;
      case 15:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s15;
        break;
      case 20:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s20;
        break;
      case 25:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s25;
        break;
      case 30:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s30;
        break;
      case 35:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s35;
        break;
      case 40:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s40;
        break;
      case 45:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s45;
        break;
      case 50:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s50;
        break;
      case 55:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s55;
        break;
      case 60:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s60;
        break;
      case 120:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s120;
        break;
      case 180:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s180;
        break;
      case 240:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s240;
        break;
      case 900:
        ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s900;
        break;
      default:
        report_fatal_error("Invalid ntn_ul_sync_validity_dur {}.", sib19_params.ntn_ul_sync_validity_dur.value());
    }
  }

  // Cell-specific-k-offset.
  if (sib19_params.cell_specific_koffset.has_value()) {
    ntn_cfg.cell_specific_koffset_r17_present = true;
    ntn_cfg.cell_specific_koffset_r17         = sib19_params.cell_specific_koffset.value();
  }

  // K-mac.
  if (sib19_params.k_mac.has_value()) {
    ntn_cfg.kmac_r17_present = true;
    ntn_cfg.kmac_r17         = sib19_params.k_mac.value();
  }

  // TA-Info. Exempt from SI change notification or valueTag modification in SIB1.
  if (sib19_params.ta_info.has_value()) {
    ntn_cfg.ta_info_r17_present                 = true;
    ta_info_r17_s& ta_info                      = ntn_cfg.ta_info_r17;
    ta_info.ta_common_drift_r17_present         = true;
    ta_info.ta_common_drift_variant_r17_present = true;
    ta_info.ta_common_r17                       = static_cast<uint32_t>(sib19_params.ta_info->ta_common / 0.004072);
    ta_info.ta_common_drift_r17                 = static_cast<int32_t>(sib19_params.ta_info->ta_common_drift / 0.0002);
    ta_info.ta_common_drift_variant_r17 =
        static_cast<uint16_t>(sib19_params.ta_info->ta_common_drift_variant / 0.00002);
    ta_info.ta_common_r17 += static_cast<int32_t>(sib19_params.ta_info->ta_common_offset / 0.004072);
  }

  if (sib19_params.polarization.has_value()) {
    if (sib19_params.polarization->dl.has_value()) {
      ntn_cfg.ntn_polarization_dl_r17_present = true;
      if (sib19_params.polarization->dl.value() == ntn_polarization_t::polarization_type::lhcp) {
        ntn_cfg.ntn_polarization_dl_r17 = ntn_cfg_r17_s::ntn_polarization_dl_r17_opts::lhcp;
      } else if (sib19_params.polarization->dl.value() == ntn_polarization_t::polarization_type::rhcp) {
        ntn_cfg.ntn_polarization_dl_r17 = ntn_cfg_r17_s::ntn_polarization_dl_r17_opts::rhcp;
      } else {
        ntn_cfg.ntn_polarization_dl_r17 = ntn_cfg_r17_s::ntn_polarization_dl_r17_opts::linear;
      }
    }
    if (sib19_params.polarization->ul.has_value()) {
      ntn_cfg.ntn_polarization_ul_r17_present = true;
      if (sib19_params.polarization->ul.value() == ntn_polarization_t::polarization_type::lhcp) {
        ntn_cfg.ntn_polarization_ul_r17 = ntn_cfg_r17_s::ntn_polarization_ul_r17_opts::lhcp;
      } else if (sib19_params.polarization->ul.value() == ntn_polarization_t::polarization_type::rhcp) {
        ntn_cfg.ntn_polarization_ul_r17 = ntn_cfg_r17_s::ntn_polarization_ul_r17_opts::rhcp;
      } else {
        ntn_cfg.ntn_polarization_ul_r17 = ntn_cfg_r17_s::ntn_polarization_ul_r17_opts::linear;
      }
    }
  }

  // Ephemeris info. Exempt from SI change notification or valueTag modification in SIB1.
  if (sib19_params.ephemeris_info.has_value()) {
    ntn_cfg.ephemeris_info_r17_present = true;
    if (const auto* pos_vel = std::get_if<ecef_coordinates_t>(&sib19_params.ephemeris_info.value())) {
      ntn_cfg.ephemeris_info_r17.set_position_velocity_r17();
      position_velocity_r17_s& rv = ntn_cfg.ephemeris_info_r17.position_velocity_r17();
      rv.position_x_r17           = static_cast<int32_t>(pos_vel->position_x / 1.3);
      rv.position_y_r17           = static_cast<int32_t>(pos_vel->position_y / 1.3);
      rv.position_z_r17           = static_cast<int32_t>(pos_vel->position_z / 1.3);
      rv.velocity_vx_r17          = static_cast<int32_t>(pos_vel->velocity_vx / 0.06);
      rv.velocity_vy_r17          = static_cast<int32_t>(pos_vel->velocity_vy / 0.06);
      rv.velocity_vz_r17          = static_cast<int32_t>(pos_vel->velocity_vz / 0.06);
    } else {
      const auto& orbital_elem = std::get<orbital_coordinates_t>(sib19_params.ephemeris_info.value());
      ntn_cfg.ephemeris_info_r17.set_orbital_r17();
      orbital_r17_s& orbit      = ntn_cfg.ephemeris_info_r17.orbital_r17();
      orbit.semi_major_axis_r17 = static_cast<uint64_t>((orbital_elem.semi_major_axis - 6500000) / 0.004249);
      orbit.eccentricity_r17    = static_cast<uint32_t>(orbital_elem.eccentricity / 0.00000001431);
      orbit.inclination_r17     = static_cast<int32_t>(orbital_elem.inclination / 0.00000002341);
      orbit.longitude_r17       = static_cast<uint32_t>(orbital_elem.longitude / 0.00000002341);
      orbit.periapsis_r17       = static_cast<uint32_t>(orbital_elem.periapsis / 0.00000002341);
      orbit.mean_anomaly_r17    = static_cast<uint32_t>(orbital_elem.mean_anomaly / 0.00000002341);
    }
  }

  // TA-Report.
  if (sib19_params.ta_report.has_value()) {
    ntn_cfg.ta_report_r17_present = sib19_params.ta_report.value();
  }

  return ntn_cfg;
}

sib19_r17_s ocudu::odu::make_asn1_rrc_cell_sib19(const sib19_info& sib19_params)
{
  sib19_r17_s sib19;

  // Distance Threshold.
  sib19.distance_thresh_r17_present = false;

  // T-Service, currently not supported.
  sib19.t_service_r17_present = false;

  // NTN-Config
  sib19.ntn_cfg_r17_present = true;
  sib19.ntn_cfg_r17         = make_asn1_rrc_cell_ntn_cfg(sib19_params);

  make_asn1_rrc_advanced_cell_sib19(sib19_params, sib19);

  return sib19;
}

#ifndef OCUDU_HAS_ENTERPRISE_NTN

void ocudu::odu::make_asn1_rrc_advanced_cell_sib19(const sib19_info& sib19_params, sib19_r17_s& out)
{
  // Encoding of the advanced NTN config parameters are not implemented.
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
