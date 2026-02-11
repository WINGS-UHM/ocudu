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

#include "ocudu/du/du_cell_config_helpers.h"
#include "ocudu/scheduler/config/srs_builder_params.h"
#include <ocudu/du/du_cell_config.h>
#include <ocudu/ran/srs/srs_configuration.h>
#include <optional>

namespace ocudu {
namespace odu {
namespace du_srs_mng_details {

/// Helper that computes the SRS bandwidth parameter \f$C_{SRS}\f$ based on the number of UL BWP RBs.
/// /// \param [in] nof_ul_bwp_rbs Number of RBs in the UL BWP.
std::optional<unsigned> compute_c_srs(unsigned nof_ul_bwp_rbs);

/// \brief Helper that returns the PRB start value for the SRS within the UP BWP.
/// \param[in] c_srs \f$C_{SRS}\f$ value, as per Section 6.4.1.4, TS 38.211.
/// \param[in] nof_ul_bwp_rbs Number of RBs in the UL BWP.
/// \return The PRB start value for the SRS within the UP BWP; this value is computed in such a way that the SRS
///         resources are placed at the center of the BWP.
unsigned compute_srs_rb_start(unsigned c_srs, unsigned nof_ul_bwp_rbs);

/// \brief Helper that updates the starting SRS config with user-defined parameters.
/// This is not the UE-specific SRS configuration, but a default template with user-defined parameters that is used to
/// build the UE-specific SRS-Config.
/// \param[in] cell_cfg DU Cell configuration.
/// \return The default SRS configuration (not yet UE-specific) later used to build the UE-specific SRS-Config.
inline srs_config build_default_srs_cfg(const du_cell_config& cell_cfg)
{
  srs_config srs_cfg = config_helpers::make_srs_config(cell_cfg.init_bwp_builder.srs_cfg, cell_cfg.pci);
  ocudu_assert(srs_cfg.srs_res_list.size() == 1 and srs_cfg.srs_res_set_list.size() == 1,
               "The SRS resource list and the SRS resource set list are expected to have a single element");
  return srs_cfg;
}

} // namespace du_srs_mng_details
} // namespace odu
} // namespace ocudu
