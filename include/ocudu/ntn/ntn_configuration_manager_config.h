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

namespace ocudu {
namespace ocudu_ntn {

/// NTN assistance information (SIB19 and ephemeris propagation).
struct ntn_assistance_info {
  /// SIB19 fields exempt from SIB1 valuetag (changes don't trigger SI change notifications).
  /// Moving reference location for NTN Earth-moving cell. Exempt from SI change determination per TS 38.331.
  std::optional<geodetic_coordinates_t> moving_reference_location;
  /// Ephemeris info. Exempt from SI change determination per TS 38.331.
  std::variant<ecef_coordinates_t, orbital_coordinates_t> ephemeris_info;
  /// Timing advance info (ta-Common, ta-CommonDrift, ta-CommonDriftVariant). Exempt from SI change determination.
  std::optional<ta_info_t> ta_info;
  /// Epoch time for NTN assistance info. Exempt from SI change determination.
  std::optional<epoch_time_t> epoch_time;
  /// Validity duration for UL sync assistance info in seconds. Exempt from SI change determination.
  std::optional<unsigned> ntn_ul_sync_validity_dur;

  /// SIB19 fields tracked by SIB1 valuetag (changes require valuetag modification).
  /// Reference location for NTN quasi-Earth fixed cell (in degrees).
  std::optional<geodetic_coordinates_t> reference_location;
  /// Distance threshold from serving cell reference location. Each step represents 50m.
  std::optional<unsigned> distance_threshold;
  /// Time when cell stops serving current area (Unix time in ms since 1970-01-01).
  std::optional<uint64_t> t_service;
  /// Cell-specific scheduling offset (k_offset) for NTN, in milliseconds.
  std::chrono::milliseconds cell_specific_koffset;
  /// Scheduling offset k_mac if DL/UL frame timing not aligned.
  std::optional<unsigned> k_mac;
  /// Polarization info for service link.
  std::optional<ntn_polarization_t> polarization;
  /// Indicates if timing advance reporting is enabled.
  std::optional<bool> ta_report;

  /// Metadata fields (not directly in SIB19, used for SIB19 generation):
  /// Epoch timestamp (Unix time in ms since 1970-01-01) used to calculate epochTime.
  std::optional<uint64_t> epoch_timestamp;
  /// Offset in SFN between SIB19 transmission and epoch time.
  std::optional<uint64_t> epoch_sfn_offset;
  /// Use ECEF state vectors (true) or ECI orbital parameters (false) for ephemeris format.
  std::optional<bool> use_state_vector;
  /// Feeder link parameters for Doppler computation (backend).
  std::optional<feeder_link_info_t> feeder_link_info;
  /// Gateway location (in degrees) for backend processing.
  std::optional<geodetic_coordinates_t> ntn_gateway_location;
};

/// NTN Cell configuration.
struct ntn_cell_config {
  /// NR-CGI.
  nr_cell_global_id_t nr_cgi;
  /// Sector Id (4-14 bits).
  std::optional<unsigned> sector_id;
  /// SIB19 scheduling information.
  unsigned si_msg_idx;
  unsigned si_period_rf;
  unsigned si_window_len_slots;
  unsigned si_window_position;
  /// NTN assistance information.
  ntn_assistance_info assistance_info;
};

/// NTN Configuration manager config.
struct ntn_configuration_manager_config {
  /// NTN Cell configuration.
  std::vector<ntn_cell_config> cells;
};

} // namespace ocudu_ntn
} // namespace ocudu
