/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_high_ntn_config_yaml_writer.h"
#include "du_high_unit_cell_ntn_config.h"

using namespace ocudu;

void ocudu::fill_ntn_config_in_yaml_schema(YAML::Node& node, const du_high_unit_cell_ntn_config& config)
{
  auto ntn_node = node["ntn"];

  ntn_node["cell_specific_koffset"] = config.cell_specific_koffset.count();

  if (config.ntn_ul_sync_validity_dur) {
    ntn_node["ntn_ul_sync_validity_dur"] = config.ntn_ul_sync_validity_dur.value();
  }

  if (config.ta_info) {
    YAML::Node ta_info_node;
    ta_info_node["ta_common"]               = config.ta_info.value().ta_common;
    ta_info_node["ta_common_drift"]         = config.ta_info.value().ta_common_drift;
    ta_info_node["ta_common_drift_variant"] = config.ta_info.value().ta_common_drift_variant;
    ta_info_node["ta_common_offset"]        = config.ta_info.value().ta_common_offset;

    ntn_node["ta_info"] = ta_info_node;
  }

  if (config.epoch_timestamp) {
    ntn_node["epoch_timestamp"] = config.epoch_timestamp.value();
  }

  if (config.feeder_link_info) {
    YAML::Node fl_node;
    fl_node["enable_doppler_compensation"] = config.feeder_link_info.value().enable_doppler_compensation;
    fl_node["dl_freq"]                     = config.feeder_link_info.value().dl_freq;
    fl_node["ul_freq"]                     = config.feeder_link_info.value().ul_freq;

    ntn_node["feeder_link_info"] = fl_node;
  }

  if (config.ntn_gateway_location) {
    YAML::Node gw_loc_node;
    gw_loc_node["latitude"]  = config.ntn_gateway_location.value().latitude;
    gw_loc_node["longitude"] = config.ntn_gateway_location.value().longitude;
    gw_loc_node["altitude"]  = config.ntn_gateway_location.value().altitude;

    ntn_node["ntn_gateway_location"] = gw_loc_node;
  }

  if (config.epoch_time.has_value()) {
    YAML::Node epoch_node;
    epoch_node["sfn"]             = config.epoch_time.value().sfn;
    epoch_node["subframe_number"] = config.epoch_time.value().subframe_number;

    ntn_node["epoch_time"] = epoch_node;
  }

  if (config.epoch_sfn_offset) {
    ntn_node["epoch_sfn_offset"] = config.epoch_sfn_offset.value();
  }

  if (config.use_state_vector) {
    ntn_node["use_state_vector"] = config.use_state_vector.value();
  }

  if (std::holds_alternative<ecef_coordinates_t>(config.ephemeris_info)) {
    YAML::Node  ephemeris_node;
    const auto& ephem       = std::get<ecef_coordinates_t>(config.ephemeris_info);
    ephemeris_node["pos_x"] = ephem.position_x;
    ephemeris_node["pos_y"] = ephem.position_y;
    ephemeris_node["pos_z"] = ephem.position_z;
    ephemeris_node["vel_x"] = ephem.velocity_vx;
    ephemeris_node["vel_y"] = ephem.velocity_vy;
    ephemeris_node["vel_z"] = ephem.velocity_vz;

    ntn_node["ephemeris_info_ecef"] = ephemeris_node;
  }

  if (std::holds_alternative<orbital_coordinates_t>(config.ephemeris_info)) {
    YAML::Node  orb_node;
    const auto& orb             = std::get<orbital_coordinates_t>(config.ephemeris_info);
    orb_node["semi_major_axis"] = orb.semi_major_axis;
    orb_node["eccentricity"]    = orb.eccentricity;
    orb_node["periapsis"]       = orb.periapsis;
    orb_node["longitude"]       = orb.longitude;
    orb_node["inclination"]     = orb.inclination;
    orb_node["mean_anomaly"]    = orb.mean_anomaly;

    ntn_node["ephemeris_orbital"] = orb_node;
  }
}
