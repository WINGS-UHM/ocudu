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

sib19_r17_s ocudu::odu::make_asn1_rrc_cell_sib19(const sib19_info& sib19_params)
{
  sib19_r17_s sib19;

  // Distance Threshold.
  sib19.distance_thresh_r17_present = false;

  // T-Service, currently not supported.
  sib19.t_service_r17_present = false;

  // NTN-Config
  if (sib19_params.ntn_cfg.has_value()) {
    sib19.ntn_cfg_r17_present = true;
    sib19.ntn_cfg_r17         = make_asn1_rrc_cell_ntn_cfg(*sib19_params.ntn_cfg);
  }

  make_asn1_rrc_advanced_cell_sib19(sib19_params, sib19);

  return sib19;
}

ntn_cfg_r17_s ocudu::odu::make_asn1_rrc_cell_ntn_cfg(const ntn_config& ntn_cfg)
{
  ntn_cfg_r17_s out_ntn_cfg;

  // Epoch Time. Exempt from SI change notification or valueTag modification in SIB1.
  if (ntn_cfg.epoch_time.has_value()) {
    out_ntn_cfg.epoch_time_r17_present          = true;
    out_ntn_cfg.epoch_time_r17.sfn_r17          = ntn_cfg.epoch_time.value().sfn;
    out_ntn_cfg.epoch_time_r17.sub_frame_nr_r17 = ntn_cfg.epoch_time.value().subframe_number;
  }

  // Validity duration for UL sync assistance info in seconds.
  // Exempt from SI change notification or valueTag modification in SIB1.
  if (ntn_cfg.ntn_ul_sync_validity_dur.has_value()) {
    out_ntn_cfg.ntn_ul_sync_validity_dur_r17_present = true;
    switch (ntn_cfg.ntn_ul_sync_validity_dur.value()) {
      case 5:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s5;
        break;
      case 10:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s10;
        break;
      case 15:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s15;
        break;
      case 20:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s20;
        break;
      case 25:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s25;
        break;
      case 30:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s30;
        break;
      case 35:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s35;
        break;
      case 40:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s40;
        break;
      case 45:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s45;
        break;
      case 50:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s50;
        break;
      case 55:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s55;
        break;
      case 60:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s60;
        break;
      case 120:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s120;
        break;
      case 180:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s180;
        break;
      case 240:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s240;
        break;
      case 900:
        out_ntn_cfg.ntn_ul_sync_validity_dur_r17.value = ntn_cfg_r17_s::ntn_ul_sync_validity_dur_r17_opts::s900;
        break;
      default:
        report_fatal_error("Invalid ntn_ul_sync_validity_dur {}.", ntn_cfg.ntn_ul_sync_validity_dur.value());
    }
  }

  // Cell-specific-k-offset.
  if (ntn_cfg.cell_specific_koffset.has_value()) {
    out_ntn_cfg.cell_specific_koffset_r17_present = true;
    out_ntn_cfg.cell_specific_koffset_r17         = ntn_cfg.cell_specific_koffset->count();
  }

  // K-mac.
  if (ntn_cfg.k_mac.has_value()) {
    out_ntn_cfg.kmac_r17_present = true;
    out_ntn_cfg.kmac_r17         = ntn_cfg.k_mac.value();
  }

  // TA-Info. Exempt from SI change notification or valueTag modification in SIB1.
  if (ntn_cfg.ta_info.has_value()) {
    out_ntn_cfg.ta_info_r17_present             = true;
    ta_info_r17_s& ta_info                      = out_ntn_cfg.ta_info_r17;
    ta_info.ta_common_drift_r17_present         = true;
    ta_info.ta_common_drift_variant_r17_present = true;
    ta_info.ta_common_r17                       = static_cast<uint32_t>(ntn_cfg.ta_info->ta_common / 0.004072);
    ta_info.ta_common_drift_r17                 = static_cast<int32_t>(ntn_cfg.ta_info->ta_common_drift / 0.0002);
    ta_info.ta_common_drift_variant_r17 = static_cast<uint16_t>(ntn_cfg.ta_info->ta_common_drift_variant / 0.00002);
    ta_info.ta_common_r17 += static_cast<int32_t>(ntn_cfg.ta_info->ta_common_offset / 0.004072);
  }

  if (ntn_cfg.polarization.has_value()) {
    if (ntn_cfg.polarization->dl.has_value()) {
      out_ntn_cfg.ntn_polarization_dl_r17_present = true;
      if (ntn_cfg.polarization->dl.value() == ntn_polarization_t::polarization_type::lhcp) {
        out_ntn_cfg.ntn_polarization_dl_r17 = ntn_cfg_r17_s::ntn_polarization_dl_r17_opts::lhcp;
      } else if (ntn_cfg.polarization->dl.value() == ntn_polarization_t::polarization_type::rhcp) {
        out_ntn_cfg.ntn_polarization_dl_r17 = ntn_cfg_r17_s::ntn_polarization_dl_r17_opts::rhcp;
      } else {
        out_ntn_cfg.ntn_polarization_dl_r17 = ntn_cfg_r17_s::ntn_polarization_dl_r17_opts::linear;
      }
    }
    if (ntn_cfg.polarization->ul.has_value()) {
      out_ntn_cfg.ntn_polarization_ul_r17_present = true;
      if (ntn_cfg.polarization->ul.value() == ntn_polarization_t::polarization_type::lhcp) {
        out_ntn_cfg.ntn_polarization_ul_r17 = ntn_cfg_r17_s::ntn_polarization_ul_r17_opts::lhcp;
      } else if (ntn_cfg.polarization->ul.value() == ntn_polarization_t::polarization_type::rhcp) {
        out_ntn_cfg.ntn_polarization_ul_r17 = ntn_cfg_r17_s::ntn_polarization_ul_r17_opts::rhcp;
      } else {
        out_ntn_cfg.ntn_polarization_ul_r17 = ntn_cfg_r17_s::ntn_polarization_ul_r17_opts::linear;
      }
    }
  }

  // Ephemeris info. Exempt from SI change notification or valueTag modification in SIB1.
  if (ntn_cfg.ephemeris_info.has_value()) {
    out_ntn_cfg.ephemeris_info_r17_present = true;
    if (const auto* pos_vel = std::get_if<ecef_coordinates_t>(&ntn_cfg.ephemeris_info.value())) {
      out_ntn_cfg.ephemeris_info_r17.set_position_velocity_r17();
      position_velocity_r17_s& rv = out_ntn_cfg.ephemeris_info_r17.position_velocity_r17();
      rv.position_x_r17           = static_cast<int32_t>(pos_vel->position_x / 1.3);
      rv.position_y_r17           = static_cast<int32_t>(pos_vel->position_y / 1.3);
      rv.position_z_r17           = static_cast<int32_t>(pos_vel->position_z / 1.3);
      rv.velocity_vx_r17          = static_cast<int32_t>(pos_vel->velocity_vx / 0.06);
      rv.velocity_vy_r17          = static_cast<int32_t>(pos_vel->velocity_vy / 0.06);
      rv.velocity_vz_r17          = static_cast<int32_t>(pos_vel->velocity_vz / 0.06);
    } else {
      const auto& orbital_elem = std::get<orbital_coordinates_t>(ntn_cfg.ephemeris_info.value());
      out_ntn_cfg.ephemeris_info_r17.set_orbital_r17();
      orbital_r17_s& orbit      = out_ntn_cfg.ephemeris_info_r17.orbital_r17();
      orbit.semi_major_axis_r17 = static_cast<uint64_t>((orbital_elem.semi_major_axis - 6500000) / 0.004249);
      orbit.eccentricity_r17    = static_cast<uint32_t>(orbital_elem.eccentricity / 0.00000001431);
      orbit.inclination_r17     = static_cast<int32_t>(orbital_elem.inclination / 0.00000002341);
      orbit.longitude_r17       = static_cast<uint32_t>(orbital_elem.longitude / 0.00000002341);
      orbit.periapsis_r17       = static_cast<uint32_t>(orbital_elem.periapsis / 0.00000002341);
      orbit.mean_anomaly_r17    = static_cast<uint32_t>(orbital_elem.mean_anomaly / 0.00000002341);
    }
  }

  // TA-Report.
  if (ntn_cfg.ta_report.has_value()) {
    out_ntn_cfg.ta_report_r17_present = ntn_cfg.ta_report.value();
  }

  return out_ntn_cfg;
}

#ifndef OCUDU_HAS_ENTERPRISE_NTN

void ocudu::odu::make_asn1_rrc_advanced_cell_sib19(const sib19_info& sib19_params, sib19_r17_s& out)
{
  // Encoding of the advanced NTN config parameters are not implemented.
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
