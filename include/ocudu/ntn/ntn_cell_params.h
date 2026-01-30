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

/// NTN cell-level configuration parameters.
struct ntn_cell_params {
  /// Scheduling offset used for timing relationships modified for NTN operation (see TS 38.213 and TS 38.300,
  /// Section 16.14.2). The unit is milliseconds.
  ///
  /// \note In the specifications, the K_offset field is expressed as a number of slots assuming a subcarrier spacing of
  /// 15 kHz (i.e., 1 slot = 1 ms). To avoid ambiguity with other subcarrier spacings, this parameter is represented in
  /// the implementation as std::chrono::milliseconds.
  std::chrono::milliseconds cell_specific_koffset = std::chrono::milliseconds(0);

  /// Scheduling offset provided by network if downlink and uplink frame timing are not aligned at gNB.
  std::optional<uint16_t> k_mac;

  /// Validity duration for UL synchronization assistance information.
  std::optional<uint16_t> ntn_ul_sync_validity_dur;

  /// Reference epoch time for NTN assistance information.
  std::optional<epoch_time_t> epoch_time;

  /// Satellite ephemeris information.
  std::optional<std::variant<ecef_coordinates_t, orbital_coordinates_t>> ephemeris_info;

  /// Timing advance information for NTN.
  std::optional<ta_info_t> ta_info;

  /// Polarization information for downlink/uplink transmission on service link.
  std::optional<ntn_polarization_t> polarization;

  /// Enable timing advance reporting from UEs.
  std::optional<bool> ta_report;

  /// Whether UL HARQ Mode B is enabled for this NTN cell (if there is at least one UL HARQ process in mode B).
  bool ul_harq_mode_b = false;

  /// Helper method to check if NTN is enabled.
  bool is_enabled() const { return cell_specific_koffset.count() > 0; }
};

} // namespace ocudu
