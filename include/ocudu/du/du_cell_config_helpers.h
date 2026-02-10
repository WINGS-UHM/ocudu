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
#include "ocudu/ran/ssb/ssb_mapping.h"
#include "ocudu/scheduler/config/cell_config_builder_params.h"
#include "ocudu/scheduler/config/rlm_helper.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"

namespace ocudu {
namespace config_helpers {

/// Builds a DU-level UE-dedicated serving cell configuration from a full UE serving cell configuration.
inline odu::du_ue_ded_serv_cell_config make_du_ue_ded_serv_cell_config(const serving_cell_config& ue_serv_cell_cfg);
/// Builds PDSCH builder parameters from a full UE serving cell configuration.
inline pdsch_builder_params make_pdsch_builder_params(const serving_cell_config& ue_serv_cell_cfg);
/// Builds PUSCH builder parameters from a full UE serving cell configuration.
inline pusch_builder_params make_pusch_builder_params(const serving_cell_config& ue_serv_cell_cfg);
/// Builds a PDSCH serving cell configuration from PDSCH builder parameters.
inline pdsch_serving_cell_config make_pdsch_serv_cell_config(const pdsch_builder_params& pdsch_params);
/// Builds a PUSCH serving cell configuration from PUSCH builder parameters.
inline pusch_serving_cell_config make_pusch_serv_cell_config(const pusch_builder_params& pusch_params);

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
  cfg.init_bwp_builder.pdsch             = make_pdsch_builder_params(ue_serv_cell);
  cfg.init_bwp_builder.pusch             = make_pusch_builder_params(ue_serv_cell);

  return cfg;
}

/// Builds a DU-level UE-dedicated serving cell configuration from a full UE serving cell configuration.
inline odu::du_ue_ded_serv_cell_config make_du_ue_ded_serv_cell_config(const serving_cell_config& ue_serv_cell_cfg)
{
  odu::du_ue_ded_serv_cell_config cfg{};
  cfg.pdcch_cfg    = ue_serv_cell_cfg.init_dl_bwp.pdcch_cfg;
  cfg.csi_meas_cfg = ue_serv_cell_cfg.csi_meas_cfg;
  if (ue_serv_cell_cfg.ul_config.has_value()) {
    cfg.pucch_cfg = ue_serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg;
    cfg.srs_cfg   = ue_serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg;
  }
  return cfg;
}

/// Builds a Radio Link Monitoring configuration from DU cell configuration.
inline std::optional<radio_link_monitoring_config> make_rlm_config(const odu::du_cell_config& du_cell_cfg)
{
  const rlm_resource_type rlm_type = du_cell_cfg.init_bwp_builder.rlm.type;
  if (rlm_type == rlm_resource_type::default_type) {
    return std::nullopt;
  }

  const uint8_t l_max =
      ssb_get_L_max(du_cell_cfg.ssb_cfg.scs, du_cell_cfg.dl_carrier.arfcn_f_ref, du_cell_cfg.dl_carrier.band);

  rlm_helper::rlm_builder_params rlm_params;
  if (rlm_type == rlm_resource_type::ssb || rlm_type == rlm_resource_type::ssb_and_csi_rs) {
    rlm_params =
        rlm_helper::rlm_builder_params(rlm_type, l_max, du_cell_cfg.ssb_cfg.ssb_bitmap, du_cell_cfg.ssb_cfg.beam_ids);
  } else {
    rlm_params = rlm_helper::rlm_builder_params(rlm_type, l_max);
  }

  span<const nzp_csi_rs_resource> csi_rs_resources;
  if (du_cell_cfg.ue_ded_serv_cell_cfg.csi_meas_cfg.has_value()) {
    csi_rs_resources = du_cell_cfg.ue_ded_serv_cell_cfg.csi_meas_cfg->nzp_csi_rs_res_list;
  }

  radio_link_monitoring_config rlm_cfg = rlm_helper::make_radio_link_monitoring_config(rlm_params, csi_rs_resources);
  if (rlm_cfg.rlm_resources.empty()) {
    return std::nullopt;
  }
  return rlm_cfg;
}

/// Builds a PDSCH configuration from DU cell configuration.
inline std::optional<pdsch_config> make_pdsch_config(const odu::du_cell_config& du_cell_cfg)
{
  pdsch_config pdsch_cfg;

  pdsch_cfg.pdsch_mapping_type_a_dmrs.emplace();
  pdsch_cfg.pdsch_mapping_type_a_dmrs->additional_positions = du_cell_cfg.init_bwp_builder.pdsch.additional_positions;

  pdsch_cfg.tci_states.push_back(tci_state{
      .state_id  = static_cast<tci_state_id_t>(0),
      .qcl_type1 = {.ref_sig  = {.type = qcl_info::reference_signal::reference_signal_type::ssb,
                                 .ssb  = static_cast<ssb_id_t>(0)},
                    .qcl_type = qcl_info::qcl_type::type_d},
  });

  pdsch_cfg.res_alloc = pdsch_config::resource_allocation::resource_allocation_type_1;
  pdsch_cfg.rbg_sz    = rbg_size::config1;

  pdsch_cfg.vrb_to_prb_interleaving = du_cell_cfg.init_bwp_builder.pdsch.interleaving_bundle_size;
  pdsch_cfg.mcs_table               = du_cell_cfg.init_bwp_builder.pdsch.mcs_table;

  pdsch_cfg.harq_process_num_size_dci_1_1 = pdsch_config::harq_process_num_dci_1_1_size::n4;
  pdsch_cfg.harq_process_num_size_dci_1_2 = pdsch_config::harq_process_num_dci_1_2_size::n4;

  // According to TS 38.214 Section 5.1.2.3, prb-BundlingType size must match the VRB-to-PRB mapping type.
  switch (pdsch_cfg.vrb_to_prb_interleaving) {
    case vrb_to_prb::mapping_type::non_interleaved:
      // > If $P'_{BWP,i}$ is determined as "wideband", the UE is not expected to be scheduled with non-contiguous
      // > PRBs and the UE may assume that the same precoding is applied to the allocated resource.
      pdsch_cfg.prb_bndlg.bundling.emplace<prb_bundling::static_bundling>(
          prb_bundling::static_bundling({.sz = prb_bundling::static_bundling::bundling_size::wideband}));
      break;
    case vrb_to_prb::mapping_type::interleaved_n2:
      // > When a UE is configured with nominal RBG size = 2 for bandwidth part i according to clause 5.1.2.2.1, or
      // > when a UE is configured with interleaving unit of 2 for VRB to PRB mapping provided by the higher layer
      // > parameter vrb-ToPRB-Interleaver given by PDSCH-Config for bandwidth part i, the UE is not expected to be
      // > configured with $P'_{BWP,i} = 4$.
      pdsch_cfg.prb_bndlg.bundling.emplace<prb_bundling::static_bundling>(
          prb_bundling::static_bundling({.sz = std::nullopt}));
      break;
    case vrb_to_prb::mapping_type::interleaved_n4:
      pdsch_cfg.prb_bndlg.bundling.emplace<prb_bundling::static_bundling>(
          prb_bundling::static_bundling({.sz = prb_bundling::static_bundling::bundling_size::n4}));
      break;
  }

  pdsch_cfg.zp_csi_rs_res_list = du_cell_cfg.init_bwp_builder.pdsch.zp_csi_rs_res_list;
  pdsch_cfg.p_zp_csi_rs_res    = du_cell_cfg.init_bwp_builder.pdsch.p_zp_csi_rs_res;

  return pdsch_cfg;
}

/// Builds a PUSCH configuration from PUSCH builder parameters.
inline pusch_config make_pusch_config(const pusch_builder_params& pusch_params)
{
  pusch_config pusch_cfg = make_default_pusch_config();

  pusch_cfg.mcs_table = pusch_params.mcs_table;
  if (!pusch_cfg.pusch_mapping_type_a_dmrs.has_value()) {
    pusch_cfg.pusch_mapping_type_a_dmrs.emplace();
  }
  pusch_cfg.pusch_mapping_type_a_dmrs->additional_positions = pusch_params.additional_positions;

  if (pusch_params.transform_precoding_enabled) {
    pusch_cfg.trans_precoder = pusch_config::transform_precoder::enabled;
    pusch_cfg.pusch_mapping_type_a_dmrs->trans_precoder_enabled.emplace(
        dmrs_uplink_config::transform_precoder_enabled{std::nullopt, false, false});
  }
  if (pusch_params.uci_beta_offsets.has_value()) {
    if (!pusch_cfg.uci_cfg.has_value()) {
      pusch_cfg.uci_cfg.emplace();
    }
    pusch_cfg.uci_cfg->beta_offsets_cfg = uci_on_pusch::beta_offsets_semi_static{pusch_params.uci_beta_offsets.value()};
  }
  if (pusch_params.p0_pusch_alpha.has_value()) {
    if (!pusch_cfg.pusch_pwr_ctrl.has_value()) {
      pusch_cfg.pusch_pwr_ctrl.emplace(pusch_config::pusch_power_control{});
    }
    if (pusch_cfg.pusch_pwr_ctrl->p0_alphasets.empty()) {
      pusch_cfg.pusch_pwr_ctrl->p0_alphasets.emplace_back();
    }
    pusch_cfg.pusch_pwr_ctrl->p0_alphasets.front().p0_pusch_alpha = pusch_params.p0_pusch_alpha.value();
  }

  return pusch_cfg;
}

/// Builds PDSCH builder parameters from a full UE serving cell configuration.
inline pdsch_builder_params make_pdsch_builder_params(const serving_cell_config& ue_serv_cell_cfg)
{
  pdsch_builder_params params{};
  if (ue_serv_cell_cfg.pdsch_serv_cell_cfg.has_value()) {
    params.nof_harq_procs            = static_cast<uint8_t>(ue_serv_cell_cfg.pdsch_serv_cell_cfg->nof_harq_proc);
    params.dl_harq_feedback_disabled = ue_serv_cell_cfg.pdsch_serv_cell_cfg->dl_harq_feedback_disabled;
  }
  if (ue_serv_cell_cfg.init_dl_bwp.pdsch_cfg.has_value()) {
    const auto& pdsch_cfg           = ue_serv_cell_cfg.init_dl_bwp.pdsch_cfg.value();
    params.mcs_table                = pdsch_cfg.mcs_table;
    params.interleaving_bundle_size = pdsch_cfg.vrb_to_prb_interleaving;
    params.zp_csi_rs_res_list       = pdsch_cfg.zp_csi_rs_res_list;
    params.p_zp_csi_rs_res          = pdsch_cfg.p_zp_csi_rs_res;
    if (pdsch_cfg.pdsch_mapping_type_a_dmrs.has_value()) {
      params.additional_positions = pdsch_cfg.pdsch_mapping_type_a_dmrs->additional_positions;
    }
  }
  return params;
}

/// Builds PUSCH builder parameters from a full UE serving cell configuration.
inline pusch_builder_params make_pusch_builder_params(const serving_cell_config& ue_serv_cell_cfg)
{
  pusch_builder_params params{};
  if (ue_serv_cell_cfg.ul_config.has_value()) {
    const auto& ul_cfg = ue_serv_cell_cfg.ul_config.value();
    if (ul_cfg.pusch_serv_cell_cfg.has_value()) {
      params.nof_harq_procs = static_cast<uint8_t>(ul_cfg.pusch_serv_cell_cfg->nof_harq_proc);
      params.ul_harq_mode   = ul_cfg.pusch_serv_cell_cfg->ul_harq_mode;
      if (ul_cfg.pusch_serv_cell_cfg->cbg_tx.has_value()) {
        params.cbg_tx = static_cast<uint8_t>(ul_cfg.pusch_serv_cell_cfg->cbg_tx->max_cgb_per_tb);
      }
      params.x_ov_head = ul_cfg.pusch_serv_cell_cfg->x_ov_head;
    }
    if (ul_cfg.init_ul_bwp.pusch_cfg.has_value()) {
      const auto& pusch_cfg = ul_cfg.init_ul_bwp.pusch_cfg.value();
      params.mcs_table      = pusch_cfg.mcs_table;
      if (pusch_cfg.pusch_mapping_type_a_dmrs.has_value()) {
        params.additional_positions = pusch_cfg.pusch_mapping_type_a_dmrs->additional_positions;
      }
      if (pusch_cfg.trans_precoder == pusch_config::transform_precoder::enabled) {
        params.transform_precoding_enabled = true;
      }
      if (pusch_cfg.uci_cfg.has_value() && pusch_cfg.uci_cfg->beta_offsets_cfg.has_value()) {
        if (const auto* beta_offsets =
                std::get_if<uci_on_pusch::beta_offsets_semi_static>(&pusch_cfg.uci_cfg->beta_offsets_cfg.value())) {
          params.uci_beta_offsets = *beta_offsets;
        }
      }
      if (pusch_cfg.pusch_pwr_ctrl.has_value() && !pusch_cfg.pusch_pwr_ctrl->p0_alphasets.empty()) {
        params.p0_pusch_alpha = pusch_cfg.pusch_pwr_ctrl->p0_alphasets.front().p0_pusch_alpha;
      }
    }
  }
  return params;
}

/// Builds a PDSCH serving cell configuration from PDSCH builder parameters.
inline pdsch_serving_cell_config make_pdsch_serv_cell_config(const pdsch_builder_params& pdsch_params)
{
  pdsch_serving_cell_config cfg = make_default_pdsch_serving_cell_config();
  cfg.nof_harq_proc = static_cast<pdsch_serving_cell_config::nof_harq_proc_for_pdsch>(pdsch_params.nof_harq_procs);
  cfg.dl_harq_feedback_disabled = pdsch_params.dl_harq_feedback_disabled;
  return cfg;
}

/// Builds a PUSCH serving cell configuration from PUSCH builder parameters.
inline pusch_serving_cell_config make_pusch_serv_cell_config(const pusch_builder_params& pusch_params)
{
  pusch_serving_cell_config cfg{};
  if (pusch_params.cbg_tx) {
    cfg.cbg_tx.emplace();
    cfg.cbg_tx->max_cgb_per_tb = static_cast<
        pusch_serving_cell_config::pusch_code_block_group_transmission::max_code_block_groups_per_transport_block>(
        *pusch_params.cbg_tx);
  }
  cfg.x_ov_head     = pusch_params.x_ov_head;
  cfg.nof_harq_proc = static_cast<pusch_serving_cell_config::nof_harq_proc_for_pusch>(pusch_params.nof_harq_procs);
  cfg.ul_harq_mode  = pusch_params.ul_harq_mode;
  return cfg;
}

/// Builds a full UE serving cell configuration from DU cell configuration and a cell index.
inline serving_cell_config make_ue_serving_cell_config(const odu::du_ue_ded_serv_cell_config& ue_ded_cfg,
                                                       du_cell_index_t                        cell_index)
{
  serving_cell_config cfg{};
  cfg.cell_index            = cell_index;
  cfg.init_dl_bwp.pdcch_cfg = ue_ded_cfg.pdcch_cfg;
  cfg.pdsch_serv_cell_cfg   = make_default_pdsch_serving_cell_config();
  cfg.csi_meas_cfg          = ue_ded_cfg.csi_meas_cfg;
  if (ue_ded_cfg.pucch_cfg.has_value() || ue_ded_cfg.srs_cfg.has_value()) {
    cfg.ul_config.emplace();
    cfg.ul_config->init_ul_bwp.pucch_cfg = ue_ded_cfg.pucch_cfg;
    cfg.ul_config->init_ul_bwp.srs_cfg   = ue_ded_cfg.srs_cfg;
  }
  return cfg;
}

/// Builds a full UE serving cell configuration from DU cell configuration and a cell index.
inline serving_cell_config make_ue_serving_cell_config(const odu::du_cell_config& du_cell_cfg,
                                                       du_cell_index_t            cell_index)
{
  serving_cell_config cfg = make_ue_serving_cell_config(du_cell_cfg.ue_ded_serv_cell_cfg, cell_index);
  cfg.pdsch_serv_cell_cfg = make_pdsch_serv_cell_config(du_cell_cfg.init_bwp_builder.pdsch);
  if (!cfg.ul_config.has_value()) {
    cfg.ul_config.emplace();
    cfg.ul_config->init_ul_bwp.pucch_cfg = du_cell_cfg.ue_ded_serv_cell_cfg.pucch_cfg;
    cfg.ul_config->init_ul_bwp.srs_cfg   = du_cell_cfg.ue_ded_serv_cell_cfg.srs_cfg;
  }
  cfg.ul_config->init_ul_bwp.pusch_cfg = make_pusch_config(du_cell_cfg.init_bwp_builder.pusch);
  cfg.ul_config->pusch_serv_cell_cfg   = make_pusch_serv_cell_config(du_cell_cfg.init_bwp_builder.pusch);
  cfg.init_dl_bwp.pdsch_cfg            = make_pdsch_config(du_cell_cfg);
  cfg.init_dl_bwp.rlm_cfg              = make_rlm_config(du_cell_cfg);
  return cfg;
}

} // namespace config_helpers
} // namespace ocudu
