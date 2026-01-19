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

#include "ocudu/scheduler/config/srs_builder_params.h"
#include <ocudu/du/du_cell_config.h>
#include <ocudu/ran/srs/srs_configuration.h>
#include <optional>

namespace ocudu {
namespace odu {
namespace du_srs_mng_details {

/// Helper that computes the SRS bandwidth parameter \f$C_{SRS}\f$ based on the number of UL BWP RBs.
std::optional<unsigned> compute_c_srs(unsigned nof_ul_bwp_rbs);

/// Helper that returns the PRB start value for the SRS within the UP BWP; the value is computed in such a way that the
/// SRS resources are placed at the center of the BWP.
unsigned compute_srs_rb_start(unsigned c_srs, unsigned nof_ul_bwp_rbs);

/// Helper that updates the starting SRS config with user-defined parameters.
template <bool IsPeriodic>
srs_config build_default_srs_cfg(const du_cell_config& default_cell_cfg)
{
  ocudu_assert(default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.has_value() and
                   default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.has_value(),
               "DU cell config is not valid");

  ocudu_assert(default_cell_cfg.srs_cfg.srs_type_enabled != (IsPeriodic ? srs_type::aperiodic : srs_type::periodic),
               "Request to build {} SRS configuration, but {} parameters have been provided",
               IsPeriodic ? "periodic" : "aperiodic",
               IsPeriodic ? "aperiodic" : "periodic");

  // If SRS is not enabled, we don't need to update its configuration.
  if (default_cell_cfg.srs_cfg.srs_type_enabled == srs_type::disabled) {
    return default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.value();
  }

  auto srs_cfg = default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.value();

  ocudu_assert(srs_cfg.srs_res_list.size() == 1 and srs_cfg.srs_res_set_list.size() == 1,
               "The SRS resource list and the SRS resource set list are expected to have a single element");

  srs_config::srs_resource& res = srs_cfg.srs_res_list.back();
  // Set the SRS resource ID to 0, as there is only 1 SRS resource per UE.
  res.id.ue_res_id = static_cast<srs_config::srs_res_id>(0U);

  srs_config::srs_resource_set& res_set = srs_cfg.srs_res_set_list.back();
  if (IsPeriodic) {
    res.res_type = srs_resource_type::periodic;
    // Set offset to 0. The offset will be updated later on, when the UE is allocated the SRS resources.
    res.periodicity_and_offset.emplace(
        srs_config::srs_periodicity_and_offset{.period = default_cell_cfg.srs_cfg.srs_period_prohib_time, .offset = 0});
    res_set.res_type.emplace<srs_config::srs_resource_set::periodic_resource_type>(
        srs_config::srs_resource_set::periodic_resource_type{});
    // Set the SRS resource set ID to 0, as there is only 1 SRS resource set per UE.
    res_set.id = static_cast<srs_config::srs_res_set_id>(0U);
  } else {
    res.res_type = srs_resource_type::aperiodic;
    res_set.res_type.emplace<srs_config::srs_resource_set::aperiodic_resource_type>(
        srs_config::srs_resource_set::aperiodic_resource_type{});
    // Set the SRS resource set ID to 0, as there is only 1 SRS resource set per UE.
    res_set.id = static_cast<srs_config::srs_res_set_id>(0U);
  }

  return srs_cfg;
}

} // namespace du_srs_mng_details
} // namespace odu
} // namespace ocudu
