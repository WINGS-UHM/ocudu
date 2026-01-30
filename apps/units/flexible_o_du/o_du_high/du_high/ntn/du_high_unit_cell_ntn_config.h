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

#include "ocudu/ran/ntn.h"
#include <chrono>
#include <optional>
#include <variant>

namespace ocudu {

struct du_high_unit_cell_ntn_config {
  /// Reference location of the serving cell in geodetic coordinates format (in degrees).
  std::optional<geodetic_coordinates_t> reference_location;
  /// Distance from the serving cell reference location, as defined in TS 38.304. Each step represents 50m.
  std::optional<unsigned> distance_threshold;
  /// Indicates the time information on when a cell provided via NTN is going to stop serving the area it is currently
  /// covering. It represents a timestamp in ms unit of Unix time (UTC time since 1970-01-01)
  std::optional<uint64_t> t_service;
  /// NTN-config values
  /// Indicate the epoch time for the NTN assistance information passed in the config file.
  /// It represents a timestamp in ms unit of Unix time (UTC time since 1970-01-01)
  std::optional<uint64_t> epoch_timestamp;
  /// Optional offset (in SFN) between the SIB19 transmission slot and the epoch time (EpochTime IE) of the NTN
  /// assistance info. Allows sending NTN assistance information that will become valid epoch_sfn_offset number of
  /// system frames after SIB19 Tx slot.
  std::optional<uint64_t> epoch_sfn_offset;
  /// If provided it will be used to fill the EpochTime section in SIB19.
  std::optional<epoch_time_t> epoch_time;
  /// Scheduling offset used for timing relationships modified for NTN operation (see TS 38.213 and TS 38.300,
  /// Section 16.14.2). The unit is milliseconds.
  ///
  /// \note In the specifications, the K_offset field is expressed as a number of slots assuming a subcarrier spacing of
  /// 15 kHz (i.e., 1 slot = 1 ms). To avoid ambiguity with other subcarrier spacings, this parameter is represented in
  /// the implementation as std::chrono::milliseconds.
  std::chrono::milliseconds cell_specific_koffset;
  /// Scheduling offset provided by network if downlink and uplink frame timing are not aligned at gNB.
  std::optional<unsigned> k_mac;
  /// A validity duration configured by the network for assistance information which indicates the maximum time duration
  /// (from epochTime) during which the UE can apply assistance information without having acquired new assistance
  /// information. Values {5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 120, 180, 240, 900} seconds.
  std::optional<unsigned> ntn_ul_sync_validity_dur;
  /// Whether to broadcast Ephemeris information as ECEF state vectors (if true) or ECI Orbital parameters (if false).
  /// If not provided, the value is derived from the variant of ephemeris_info.
  /// If provided and does not match the variant of ephemeris_info, the ephemeris_info is converted accordingly.
  std::optional<bool> use_state_vector;
  /// This field provides satellite ephemeris either in format of position and velocity state vector or in format of
  /// orbital parameters.
  std::variant<ecef_coordinates_t, orbital_coordinates_t> ephemeris_info;
  /// Network-controlled common timing advanced value, and it may include any timing offset considered necessary by the
  /// network.
  std::optional<ta_info_t> ta_info;
  /// Parameters of the feeder link used to compute the Doppler shifts.
  std::optional<feeder_link_info_t> feeder_link_info;
  /// Geodetic coordinates (in degrees) of the NTN Gateway location.
  std::optional<geodetic_coordinates_t> ntn_gateway_location;
  /// Indicates polarization information for downlink/uplink transmission on service link.
  std::optional<ntn_polarization_t> polarization;
  /// When this field is included in SIB19, it indicates reporting of timing advanced is enabled.
  std::optional<bool> ta_report;
};

} // namespace ocudu
