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

#include "ocudu/du/du_cell_config.h"
#include "ocudu/mac/config/mac_config_helpers.h"
#include "ocudu/ran/pdcch/pdcch_type0_css_coreset_config.h"
#include "ocudu/scheduler/config/cell_config_builder_params.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"

namespace ocudu {
namespace config_helpers {

/// Builds a DU-level UE-dedicated serving cell configuration from a full UE serving cell configuration.
inline odu::du_ue_ded_serv_cell_config make_du_ue_ded_serv_cell_config(const serving_cell_config& ue_serv_cell_cfg);

/// Generates default cell configuration used by gNB DU. The default configuration should be valid.
inline odu::du_cell_config make_default_du_cell_config(const cell_config_builder_params_extended& params = {})
{
  odu::du_cell_config cfg{};
  cfg.pci            = params.pci;
  cfg.tac            = 1;
  cfg.nr_cgi.plmn_id = plmn_identity::test_value();
  cfg.nr_cgi.nci     = nr_cell_identity::create({411, 22}, 1).value();

  cfg.dl_carrier              = make_default_dl_carrier_configuration(params);
  cfg.ul_carrier              = make_default_ul_carrier_configuration(params);
  cfg.coreset0_idx            = *params.coreset0_index;
  cfg.searchspace0_idx        = params.search_space0_index;
  cfg.dl_cfg_common           = make_default_dl_config_common(params);
  cfg.ul_cfg_common           = make_default_ul_config_common(params);
  cfg.scs_common              = params.scs_common;
  cfg.ssb_cfg                 = make_default_ssb_config(params);
  cfg.cell_barred             = false;
  cfg.intra_freq_resel        = false;
  cfg.ue_timers_and_constants = make_default_ue_timers_and_constants_config();

  // The CORESET duration of 3 symbols is only permitted if dmrs-typeA-Position is set to 3. Refer TS 38.211, 7.3.2.2.
  const pdcch_type0_css_coreset_description coreset0_desc = pdcch_type0_css_coreset_get(
      cfg.dl_carrier.band, *params.scs_ssb, params.scs_common, *params.coreset0_index, params.k_ssb->value());
  cfg.dmrs_typeA_pos = coreset0_desc.nof_symb_coreset == 3U ? dmrs_typeA_position::pos3 : dmrs_typeA_position::pos2;

  cfg.tdd_ul_dl_cfg_common               = params.tdd_ul_dl_cfg_common;
  const serving_cell_config ue_serv_cell = create_default_initial_ue_serving_cell_config(params);
  cfg.ue_ded_serv_cell_cfg               = make_du_ue_ded_serv_cell_config(ue_serv_cell);

  return cfg;
}

/// Builds a DU-level UE-dedicated serving cell configuration from a full UE serving cell configuration.
inline odu::du_ue_ded_serv_cell_config make_du_ue_ded_serv_cell_config(const serving_cell_config& ue_serv_cell_cfg)
{
  odu::du_ue_ded_serv_cell_config cfg{};
  cfg.init_dl_bwp         = ue_serv_cell_cfg.init_dl_bwp;
  cfg.ul_config           = ue_serv_cell_cfg.ul_config;
  cfg.pdsch_serv_cell_cfg = ue_serv_cell_cfg.pdsch_serv_cell_cfg;
  cfg.csi_meas_cfg        = ue_serv_cell_cfg.csi_meas_cfg;
  return cfg;
}

/// Builds a full UE serving cell configuration from DU cell configuration and a cell index.
inline serving_cell_config make_ue_serving_cell_config(const odu::du_ue_ded_serv_cell_config& ue_ded_cfg,
                                                       du_cell_index_t                        cell_index)
{
  serving_cell_config cfg{};
  cfg.cell_index          = cell_index;
  cfg.init_dl_bwp         = ue_ded_cfg.init_dl_bwp;
  cfg.ul_config           = ue_ded_cfg.ul_config;
  cfg.pdsch_serv_cell_cfg = ue_ded_cfg.pdsch_serv_cell_cfg;
  cfg.csi_meas_cfg        = ue_ded_cfg.csi_meas_cfg;
  return cfg;
}

/// Builds a full UE serving cell configuration from DU cell configuration and a cell index.
inline serving_cell_config make_ue_serving_cell_config(const odu::du_cell_config& du_cell_cfg,
                                                       du_cell_index_t            cell_index)
{
  return make_ue_serving_cell_config(du_cell_cfg.ue_ded_serv_cell_cfg, cell_index);
}

} // namespace config_helpers
} // namespace ocudu
