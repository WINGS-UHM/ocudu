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

#include <chrono>
#include <optional>
#include <variant>

namespace ocudu {

/// Satellite ephemeris in format of position and velocity state vector in ECEF.
struct ecef_coordinates_t {
  /// X coordinate of position state vector. Unit is meter.
  double position_x;
  /// Y coordinate of position state vector. Unit is meter.
  double position_y;
  /// Z coordinate of position state vector. Unit is meter.
  double position_z;
  /// X coordinate of velocity state vector. Unit is meter/second.
  double velocity_vx;
  /// Y coordinate of velocity state vector. Unit is meter/second.
  double velocity_vy;
  /// Z coordinate of velocity state vector. Unit is meter/second.
  double velocity_vz;
};

/// Satellite ephemeris in format of orbital parameters in ECI. See NIMA TR 8350.2.
struct orbital_coordinates_t {
  /// Semi major axis. Unit is meter.
  double semi_major_axis;
  /// Eccentricity. Unitless.
  double eccentricity;
  /// Argument of periapsis. Unit is radian.
  double periapsis;
  /// Longitude of ascending node. Unit is radian.
  double longitude;
  /// Mean anomaly M at epoch time. Unit is radian.
  double mean_anomaly;
  /// Inclination. Unit is radian.
  double inclination;
};

/// Timing advance information for NTN.
struct ta_info_t {
  /// Network-controlled common timing advanced value and it may include any timing offset considered necessary by the
  /// network. ta-Common with value of 0 is supported. Unit is us.
  double ta_common;
  /// Indicate drift rate of the common TA. Unit is us/s.
  double ta_common_drift;
  /// Indicate drift rate variation of the common TA. Unit is us/s^2.
  double ta_common_drift_variant;
  /// Constant offset added to the NTA-common broadcast in SIB19 to model fixed system delays independent of satellite
  /// position. Unit is us.
  double ta_common_offset;
};

/// EpochTime is used to indicate the epoch time for the NTN assistance information, and it is defined as the starting
/// time of a DL sub-frame, indicated by a SFN and a sub-frame number signaled together with the assistance information.
struct epoch_time_t {
  /// For serving cell, it indicates the current SFN or the next upcoming SFN after the frame where the message
  /// indicating the epochTime is received. For neighbour cell, it indicates the SFN nearest to the frame where the
  /// message indicating the epochTime is received.
  unsigned sfn;
  /// Sub-frame number within the SFN.
  unsigned subframe_number;
};

/// Parameters of the feeder link used to compute the Doppler shifts.
struct feeder_link_info_t {
  /// Flag to enable/disable doppler compensation for the feeder link.
  bool enable_doppler_compensation;
  /// Downlink frequency of the feeder link. Unit is Hz.
  double dl_freq;
  /// Uplink frequency of the feeder link. Unit is Hz.
  double ul_freq;
};

/// Geodetic coordinates of the NTN Gateway location.
struct geodetic_coordinates_t {
  /// Latitude. Unit is degree.
  double latitude;
  /// Longitude. Unit is degree.
  double longitude;
  /// Altitude. Unit is meter.
  double altitude;
};

/// Indicates polarization information for downlink/uplink transmission on service link.
struct ntn_polarization_t {
  enum class polarization_type { rhcp, lhcp, linear };
  /// If present, this parameter indicates polarization information for downlink transmission on service link.
  std::optional<polarization_type> dl;
  /// If present, this parameter indicates Polarization information for uplink service link.
  std::optional<polarization_type> ul;
};

} // namespace ocudu
