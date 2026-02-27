// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "du_high_ntn_config_cli11_schema.h"
#include "du_high_unit_cell_ntn_config.h"
#include "ocudu/support/cli11_utils.h"

using namespace ocudu;

#ifndef OCUDU_HAS_ENTERPRISE_NTN

void ocudu::configure_cli11_advanced_ntn_args(CLI::App& app, du_high_unit_cell_ntn_config& config)
{
  // Advanced NTN config parameters are not implemented.
}
#endif // OCUDU_HAS_ENTERPRISE_NTN

static void configure_cli11_epoch_time(CLI::App& app, epoch_time_t& epoch_time)
{
  add_option(app, "--sfn", epoch_time.sfn, "SFN Part")->capture_default_str()->check(CLI::Range(0, 1023));
  add_option(app, "--subframe_number", epoch_time.subframe_number, "Sub-frame number Part")
      ->capture_default_str()
      ->check(CLI::Range(0, 9));
}

static void configure_cli11_ta_info(CLI::App& app, ta_info_t& ta_info)
{
  add_option(app, "--ta_common", ta_info.ta_common, "TA common")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 270730.0));
  add_option(app, "--ta_common_drift", ta_info.ta_common_drift, "Drift rate of the common TA")
      ->capture_default_str()
      ->check(CLI::Range(-51.4606, 51.4606));
  add_option(app, "--ta_common_drift_variant", ta_info.ta_common_drift_variant, "Drift rate variation of the common TA")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 0.57898));
  add_option(app, "--ta_common_offset", ta_info.ta_common_offset, "Constant offset added to TA common")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 10000.0));
}

static void configure_cli11_ephemeris_info_ecef(CLI::App& app, ecef_coordinates_t& ephemeris_info)
{
  add_option(app, "--pos_x", ephemeris_info.position_x, "X Position of the satellite [m]")
      ->capture_default_str()
      ->check(CLI::Range(-43620761.6, 43620759.3));
  add_option(app, "--pos_y", ephemeris_info.position_y, "Y Position of the satellite [m]")
      ->capture_default_str()
      ->check(CLI::Range(-43620761.6, 43620759.3));
  add_option(app, "--pos_z", ephemeris_info.position_z, "Z Position of the satellite [m]")
      ->capture_default_str()
      ->check(CLI::Range(-43620761.6, 43620759.3));
  add_option(app, "--vel_x", ephemeris_info.velocity_vx, "X Velocity of the satellite [m/s]")
      ->capture_default_str()
      ->check(CLI::Range(-7864.32, 7864.26));
  add_option(app, "--vel_y", ephemeris_info.velocity_vy, "Y Velocity of the satellite [m/s]")
      ->capture_default_str()
      ->check(CLI::Range(-7864.32, 7864.26));
  add_option(app, "--vel_z", ephemeris_info.velocity_vz, "Z Velocity of the satellite [m/s]")
      ->capture_default_str()
      ->check(CLI::Range(-7864.32, 7864.26));
}

static void configure_cli11_ephemeris_info_orbital(CLI::App& app, orbital_coordinates_t& ephemeris_info)
{
  add_option(app, "--semi_major_axis", ephemeris_info.semi_major_axis, "Semi-major axis of the satellite [m]")
      ->capture_default_str()
      ->check(CLI::Range(6500000.0, 42998632.07));
  add_option(app, "--eccentricity", ephemeris_info.eccentricity, "Eccentricity of the satellite [-]")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 0.01500510825));
  add_option(app, "--periapsis", ephemeris_info.periapsis, "Periapsis of the satellite [rad]")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 6.28407400155));
  add_option(app, "--longitude", ephemeris_info.longitude, "Longitude of the satellites angle of ascending node [rad]")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 6.28407400155));
  add_option(app, "--inclination", ephemeris_info.inclination, "Inclination of the satellite [rad]")
      ->capture_default_str()
      ->check(CLI::Range(-1.57101850624, 1.57101848283));
  add_option(app, "--mean_anomaly", ephemeris_info.mean_anomaly, "Mean anomaly of the satellite [rad]")
      ->capture_default_str()
      ->check(CLI::Range(0.0, 6.28407400155));
}

static void configure_cli11_ntn_polarization(CLI::App& app, ntn_polarization_t& polarization)
{
  add_option_function<std::string>(
      app,
      "--dl",
      [&polarization](const std::string& value) {
        if (value == "lhcp") {
          polarization.dl = ntn_polarization_t::polarization_type::lhcp;
        } else if (value == "rhcp") {
          polarization.dl = ntn_polarization_t::polarization_type::rhcp;
        } else {
          polarization.dl = ntn_polarization_t::polarization_type::linear;
        }
      },
      "Polarization information for downlink transmission on service link")
      ->check(CLI::IsMember({"lhcp", "rhcp", "linear"}, CLI::ignore_case));

  add_option_function<std::string>(
      app,
      "--ul",
      [&polarization](const std::string& value) {
        if (value == "lhcp") {
          polarization.ul = ntn_polarization_t::polarization_type::lhcp;
        } else if (value == "rhcp") {
          polarization.ul = ntn_polarization_t::polarization_type::rhcp;
        } else {
          polarization.ul = ntn_polarization_t::polarization_type::linear;
        }
      },
      "Polarization information for downlink transmission on service link")
      ->check(CLI::IsMember({"lhcp", "rhcp", "linear"}, CLI::ignore_case));
}

void ocudu::configure_cli11_ntn_config_args(CLI::App& app, ntn_config& config)
{
  static epoch_time_t epoch_time;
  CLI::App*           epoch_time_subcmd = add_subcommand(app, "epoch_time", "Epoch time for the NTN assistance info");
  configure_cli11_epoch_time(*epoch_time_subcmd, epoch_time);
  epoch_time_subcmd->parse_complete_callback([&]() {
    if (app.get_subcommand("epoch_time")->count() != 0) {
      config.epoch_time = epoch_time;
    }
  });

  app.add_option_function<unsigned>(
         "--ntn_ul_sync_validity_dur",
         [&config](unsigned value) { config.ntn_ul_sync_validity_dur = value; },
         "An UL sync validity duration")
      ->check(CLI::IsMember({5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 120, 180, 240, 900}));

  app.add_option_function<unsigned>(
         "--cell_specific_koffset",
         [&config](unsigned value) { config.cell_specific_koffset = std::chrono::milliseconds(value); },
         "Cell-specific k-offset to be used for NTN [ms].")
      ->check(CLI::Range(1U, 1023U));

  static ta_info_t ta_info;
  CLI::App*        ta_info_subcmd = add_subcommand(app, "ta_info", "TA Info for the NTN assistance information");
  configure_cli11_ta_info(*ta_info_subcmd, ta_info);
  ta_info_subcmd->parse_complete_callback([&]() {
    if (app.get_subcommand("ta_info")->count() != 0) {
      config.ta_info = ta_info;
    }
  });

  static ntn_polarization_t polarization;
  CLI::App*                 polarization_subcmd =
      add_subcommand(app, "polarization", "Polarization information for downlink/uplink transmission");
  configure_cli11_ntn_polarization(*polarization_subcmd, polarization);
  polarization_subcmd->parse_complete_callback([&]() {
    if (app.get_subcommand("polarization")->count() != 0) {
      config.polarization = polarization;
    }
  });

  // Ephemeris configuration: ECEF state vector.
  static ecef_coordinates_t ecef_coordinates;
  CLI::App*                 ephem_subcmd_ecef =
      add_subcommand(app, "ephemeris_info_ecef", "Ephermeris information of the satellite in ecef coordinates");
  configure_cli11_ephemeris_info_ecef(*ephem_subcmd_ecef, ecef_coordinates);
  ephem_subcmd_ecef->parse_complete_callback([&]() {
    if (app.get_subcommand("ephemeris_info_ecef")->count() != 0) {
      config.ephemeris_info = ecef_coordinates;
    }
  });

  // Ephemeris configuration: Orbital parameters.
  static orbital_coordinates_t orbital_coordinates;
  CLI::App*                    ephem_subcmd_orbital =
      add_subcommand(app, "ephemeris_orbital", "Ephermeris information of the satellite in orbital coordinates");
  configure_cli11_ephemeris_info_orbital(*ephem_subcmd_orbital, orbital_coordinates);
  ephem_subcmd_orbital->parse_complete_callback([&]() {
    if (app.get_subcommand("ephemeris_orbital")->count() != 0) {
      config.ephemeris_info = orbital_coordinates;
    }
  });

  app.add_option_function<bool>(
      "--ta_report",
      [&config](bool value) { config.ta_report = value; },
      "When this field is included in SIB19, it indicates reporting of timing advanced is enabled");
}

static void configure_cli11_ntn_args(CLI::App& app, du_high_unit_cell_ntn_config& config)
{
  add_option(
      app, "--cell_specific_koffset", config.cell_specific_koffset, "Cell-specific k-offset to be used for NTN [ms].")
      ->capture_default_str()
      ->check(CLI::Range(1, 1023));

  app.add_option("--ntn_ul_sync_validity_dur", config.ntn_ul_sync_validity_dur, "An UL sync validity duration")
      ->capture_default_str()
      ->check(CLI::IsMember({5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 120, 180, 240, 900}));

  // Epoch time.
  static epoch_time_t epoch_time;
  CLI::App* epoch_time_subcmd = add_subcommand(app, "epoch_time", "Epoch time for the NTN assistance information");
  configure_cli11_epoch_time(*epoch_time_subcmd, epoch_time);
  epoch_time_subcmd->parse_complete_callback([&]() {
    if (app.get_subcommand("epoch_time")->count() != 0) {
      config.epoch_time = epoch_time;
    }
  });

  // TA-info
  static ta_info_t ta_info;
  CLI::App*        ta_info_subcmd = add_subcommand(app, "ta_info", "TA Info for the NTN assistance information");
  configure_cli11_ta_info(*ta_info_subcmd, ta_info);
  ta_info_subcmd->parse_complete_callback([&]() {
    if (app.get_subcommand("ta_info")->count() != 0) {
      config.ta_info = ta_info;
    }
  });

  // Ephemeris configuration: ECEF state vector.
  static ecef_coordinates_t ecef_coordinates;
  CLI::App*                 ephem_subcmd_ecef =
      add_subcommand(app, "ephemeris_info_ecef", "Ephermeris information of the satellite in ecef coordinates");
  configure_cli11_ephemeris_info_ecef(*ephem_subcmd_ecef, ecef_coordinates);
  ephem_subcmd_ecef->parse_complete_callback([&]() {
    if (app.get_subcommand("ephemeris_info_ecef")->count() != 0) {
      config.ephemeris_info = ecef_coordinates;
    }
  });

  // Ephemeris configuration: Orbital parameters.
  static orbital_coordinates_t orbital_coordinates;
  CLI::App*                    ephem_subcmd_orbital =
      add_subcommand(app, "ephemeris_orbital", "Ephermeris information of the satellite in orbital coordinates");
  configure_cli11_ephemeris_info_orbital(*ephem_subcmd_orbital, orbital_coordinates);
  ephem_subcmd_orbital->parse_complete_callback([&]() {
    if (app.get_subcommand("ephemeris_orbital")->count() != 0) {
      config.ephemeris_info = orbital_coordinates;
    }
  });

  configure_cli11_advanced_ntn_args(app, config);
}

void ocudu::configure_cli11_cell_ntn_args(CLI::App& app, std::optional<du_high_unit_cell_ntn_config>& cell_ntn_params)
{
  static du_high_unit_cell_ntn_config ntn_cfg;
  CLI::App*                           ntn_subcmd = add_subcommand(app, "ntn", "NTN configuration")->configurable();

  if (not cell_ntn_params.has_value()) {
    // Configure NTN options.
    configure_cli11_ntn_args(*ntn_subcmd, ntn_cfg);
    auto ntn_verify_callback = [&]() {
      CLI::App* ntn_sub_cmd = app.get_subcommand("ntn");
      if (ntn_sub_cmd->count() != 0) {
        cell_ntn_params = ntn_cfg;
      }
    };
    ntn_subcmd->parse_complete_callback(ntn_verify_callback);
  } else {
    configure_cli11_ntn_args(*ntn_subcmd, *cell_ntn_params);
  }
}
