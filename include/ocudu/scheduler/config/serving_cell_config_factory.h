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

#include "ocudu/ran/carrier_configuration.h"
#include "ocudu/ran/csi_rs/csi_meas_config.h"
#include "ocudu/ran/pdcch/aggregation_level.h"
#include "ocudu/ran/sib/system_info_config.h"
#include "ocudu/ran/tdd/tdd_ul_dl_config.h"
#include "ocudu/scheduler/config/cell_config_builder_params.h"
#include "ocudu/scheduler/config/csi_helper.h"
#include "ocudu/scheduler/config/serving_cell_config.h"

namespace ocudu {
namespace config_helpers {

/// Config struct that extends cell_config_builder_params with parameters that can be derived from the former.
struct cell_config_builder_params_extended : public cell_config_builder_params {
  cell_config_builder_params_extended(const cell_config_builder_params& source = {});

  /// \brief Absolute frequency of the SSB as ARFCN. This is the ARFCN of the \c SS_ref (or SSB central frequency).
  /// \c SS_ref is defined is per TS 38.104, Section 5.4.3.1 and 5.4.3.2.
  std::optional<arfcn_t> ssb_arfcn;
  unsigned               cell_nof_crbs;
};

carrier_configuration make_default_dl_carrier_configuration(const cell_config_builder_params_extended& params = {});

carrier_configuration make_default_ul_carrier_configuration(const cell_config_builder_params_extended& params = {});

coreset_configuration make_default_coreset_config(const cell_config_builder_params_extended& params = {},
                                                  coreset_id                                 cs_id  = to_coreset_id(1));

coreset_configuration make_default_coreset0_config(const cell_config_builder_params_extended& params = {});

search_space_configuration
make_default_search_space_zero_config(const cell_config_builder_params_extended& params = {});

search_space_configuration
make_default_common_search_space_config(const cell_config_builder_params_extended& params = {});

search_space_configuration make_default_ue_search_space_config(const cell_config_builder_params_extended& params = {});

bwp_configuration make_default_init_bwp(const cell_config_builder_params_extended& params = {});

dl_config_common make_default_dl_config_common(const cell_config_builder_params_extended& params = {});

ul_config_common make_default_ul_config_common(const cell_config_builder_params_extended& params = {});

ssb_configuration make_default_ssb_config(const cell_config_builder_params_extended& params = {});

uplink_config make_default_ue_uplink_config(const cell_config_builder_params_extended& params = {});

pusch_config make_default_pusch_config(const cell_config_builder_params_extended& params = {});

srs_config make_default_srs_config(const cell_config_builder_params_extended& params);

pdsch_serving_cell_config make_default_pdsch_serving_cell_config();

pdsch_config make_default_pdsch_config(const cell_config_builder_params_extended& params = {});

/// \brief Creates default CSI builder parameters.
csi_helper::csi_meas_config_builder_params
make_default_csi_builder_params(const cell_config_builder_params_extended& params = {});

pdcch_config make_ue_dedicated_pdcch_config(const cell_config_builder_params_extended& params = {});

csi_meas_config make_csi_meas_config(const cell_config_builder_params_extended& params = {});

/// \brief Creates a default UE Serving Cell configuration.
serving_cell_config
create_default_initial_ue_serving_cell_config(const cell_config_builder_params_extended& params = {});

/// \brief Creates a default UE PSCell configuration.
cell_config_dedicated
create_default_initial_ue_spcell_cell_config(const cell_config_builder_params_extended& params = {});

/// \brief Computes maximum nof. candidates that can be accommodated in a CORESET for a given aggregation level.
/// \return Maximum nof. candidates for a aggregation level.
uint8_t compute_max_nof_candidates(aggregation_level aggr_lvl, const coreset_configuration& cs_cfg);

} // namespace config_helpers
} // namespace ocudu
