/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pucch_res_test_builder_helper.h"
#include "lib/scheduler/config/cell_configuration.h"
#include "ocudu/scheduler/config/ran_cell_config_helper.h"
#include "ocudu/scheduler/config/sched_cell_config_helpers.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"

using namespace ocudu;

static std::optional<du_csi_params> derive_csi_builder_params(const serving_cell_config& base_ue_cfg)
{
  if (!base_ue_cfg.csi_meas_cfg.has_value() || base_ue_cfg.csi_meas_cfg->csi_report_cfg_list.empty()) {
    return std::nullopt;
  }

  du_csi_params params{};

  if (!base_ue_cfg.csi_meas_cfg->nzp_csi_rs_res_list.empty()) {
    const auto& meas_res            = base_ue_cfg.csi_meas_cfg->nzp_csi_rs_res_list.front();
    params.cm_csi_ofdm_symbol_index = meas_res.res_mapping.first_ofdm_symbol_in_td;
    params.pwr_ctrl_offset          = meas_res.pwr_ctrl_offset;
    if (meas_res.csi_res_period.has_value()) {
      params.csi_rs_period = *meas_res.csi_res_period;
    }
    if (meas_res.csi_res_offset.has_value()) {
      params.meas_csi_slot_offset = *meas_res.csi_res_offset;
    }
  }

  const auto& report_cfg = base_ue_cfg.csi_meas_cfg->csi_report_cfg_list.front();
  if (auto* periodic =
          std::get_if<csi_report_config::periodic_or_semi_persistent_report_on_pucch>(&report_cfg.report_cfg_type)) {
    params.enable_aperiodic_report = false;
    params.csi_report_slot_offset  = periodic->report_slot_offset;
    // Derive CSI-RS period from the configured CSI report period to preserve test configuration.
    params.csi_rs_period =
        static_cast<csi_resource_periodicity>(csi_report_periodicity_to_uint(periodic->report_slot_period));
  } else {
    params.enable_aperiodic_report = true;
    params.csi_report_slot_offset.reset();
  }

  const unsigned period_slots     = csi_resource_periodicity_to_uint(params.csi_rs_period);
  params.meas_csi_slot_offset     = params.meas_csi_slot_offset % period_slots;
  params.zp_csi_slot_offset       = params.zp_csi_slot_offset % period_slots;
  params.tracking_csi_slot_offset = params.tracking_csi_slot_offset % period_slots;
  if (params.tracking_csi_slot_offset + 1 >= period_slots) {
    params.tracking_csi_slot_offset = period_slots > 1 ? period_slots - 2 : 0;
  }
  if (params.csi_report_slot_offset.has_value()) {
    params.csi_report_slot_offset = *params.csi_report_slot_offset % period_slots;
  }

  return params;
}

static ran_cell_config generate_ran_cell_config(const bwp_uplink_common&                      init_ul_bwp,
                                                const std::optional<tdd_ul_dl_config_common>& tdd_ul_dl_cfg_common,
                                                const serving_cell_config&                    base_ue_cfg,
                                                const pucch_resource_builder_params&          pucch_cfg)
{
  ran_cell_config cell_cfg;
  if (base_ue_cfg.csi_meas_cfg.has_value() && !base_ue_cfg.csi_meas_cfg->nzp_csi_rs_res_list.empty()) {
    const auto& meas_res        = base_ue_cfg.csi_meas_cfg->nzp_csi_rs_res_list.front();
    cell_cfg.pci                = static_cast<pci_t>(meas_res.scrambling_id);
    cell_cfg.dl_carrier.nof_ant = static_cast<uint16_t>(meas_res.res_mapping.nof_ports);
  } else {
    cell_cfg.pci                = pci_t{0};
    cell_cfg.dl_carrier.nof_ant = 1;
  }
  cell_cfg.ul_cfg_common.init_ul_bwp        = init_ul_bwp;
  cell_cfg.tdd_ul_dl_cfg_common             = tdd_ul_dl_cfg_common;
  cell_cfg.init_bwp_builder.pdcch_cfg       = base_ue_cfg.init_dl_bwp.pdcch_cfg;
  cell_cfg.init_bwp_builder.pucch.resources = pucch_cfg;
  if (base_ue_cfg.ul_config.has_value() && base_ue_cfg.ul_config->init_ul_bwp.pucch_cfg.has_value() &&
      !base_ue_cfg.ul_config->init_ul_bwp.pucch_cfg->sr_res_list.empty()) {
    cell_cfg.init_bwp_builder.pucch.sr_period =
        base_ue_cfg.ul_config->init_ul_bwp.pucch_cfg->sr_res_list.front().period;
  }
  {
    pdsch_builder_params params{};
    if (base_ue_cfg.pdsch_serv_cell_cfg.has_value()) {
      params.nof_harq_procs            = static_cast<uint8_t>(base_ue_cfg.pdsch_serv_cell_cfg->nof_harq_proc);
      params.dl_harq_feedback_disabled = base_ue_cfg.pdsch_serv_cell_cfg->dl_harq_feedback_disabled;
    }
    if (base_ue_cfg.init_dl_bwp.pdsch_cfg.has_value()) {
      const auto& pdsch_cfg           = base_ue_cfg.init_dl_bwp.pdsch_cfg.value();
      params.mcs_table                = pdsch_cfg.mcs_table;
      params.interleaving_bundle_size = pdsch_cfg.vrb_to_prb_interleaving;
      if (pdsch_cfg.pdsch_mapping_type_a_dmrs.has_value()) {
        params.additional_positions = pdsch_cfg.pdsch_mapping_type_a_dmrs->additional_positions;
      }
    }
    cell_cfg.init_bwp_builder.pdsch = params;
  }
  {
    pusch_builder_params params{};
    if (base_ue_cfg.ul_config.has_value()) {
      const auto& ul_cfg = base_ue_cfg.ul_config.value();
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
    cell_cfg.init_bwp_builder.pusch = params;
  }
  cell_cfg.init_bwp_builder.csi = derive_csi_builder_params(base_ue_cfg);
  return cell_cfg;
}

pucch_res_builder_test_helper::pucch_res_builder_test_helper() : pucch_res_mgr(max_pucch_grants_per_slot) {}

pucch_res_builder_test_helper::pucch_res_builder_test_helper(
    const bwp_uplink_common&                      init_ul_bwp,
    const std::optional<tdd_ul_dl_config_common>& tdd_ul_dl_cfg_common,
    const pucch_resource_builder_params&          pucch_cfg) :
  required_info(pucch_res_builder_info{.init_ul_bwp          = init_ul_bwp,
                                       .tdd_ul_dl_cfg_common = tdd_ul_dl_cfg_common,
                                       .pucch_cfg            = pucch_cfg}),
  pucch_res_mgr(max_pucch_grants_per_slot)
{
}

pucch_res_builder_test_helper::pucch_res_builder_test_helper(const cell_configuration&            cell_cfg,
                                                             const pucch_resource_builder_params& pucch_cfg) :
  required_info(pucch_res_builder_info{.init_ul_bwp          = cell_cfg.ul_cfg_common.init_ul_bwp,
                                       .tdd_ul_dl_cfg_common = cell_cfg.tdd_cfg_common,
                                       .pucch_cfg            = pucch_cfg}),
  pucch_res_mgr(max_pucch_grants_per_slot)
{
  // Sanity check to ensure the cell_cfg and the pucch_cfg use the same parameters.
  const auto ded_pucch_resource_list = config_helpers::build_pucch_resource_list(
      pucch_cfg, cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length());
  ocudu_assert(cell_cfg.ded_pucch_resources == ded_pucch_resource_list,
               "Mismatch between the PUCCH parameters used for cell_cfg and for the UE PUCCH configuration");
}

void pucch_res_builder_test_helper::setup(const bwp_uplink_common&                      init_ul_bwp_,
                                          const std::optional<tdd_ul_dl_config_common>& tdd_ul_dl_cfg_common_,
                                          const pucch_resource_builder_params&          pucch_cfg)
{
  if (required_info.has_value()) {
    return;
  }
  required_info.emplace(pucch_res_builder_info{
      .init_ul_bwp = init_ul_bwp_, .tdd_ul_dl_cfg_common = tdd_ul_dl_cfg_common_, .pucch_cfg = pucch_cfg});
}

void pucch_res_builder_test_helper::setup(const cell_configuration&            cell_cfg,
                                          const pucch_resource_builder_params& pucch_cfg)
{
  // Sanity check to ensure the cell_cfg and the pucch_cfg use the same parameters.
  const auto ded_pucch_resource_list = config_helpers::build_pucch_resource_list(
      pucch_cfg, cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length());
  ocudu_assert(cell_cfg.ded_pucch_resources == ded_pucch_resource_list,
               "Mismatch between the PUCCH parameters used for cell_cfg and for the UE PUCCH configuration");
  setup(cell_cfg.ul_cfg_common.init_ul_bwp, cell_cfg.tdd_cfg_common, pucch_cfg);
}

bool pucch_res_builder_test_helper::add_build_new_ue_pucch_cfg(serving_cell_config& serv_cell_cfg)
{
  if (not required_info.has_value()) {
    return false;
  }

  if (not pucch_res_mgr.contains(serv_cell_cfg.cell_index)) {
    init_pucch_res_mgr(serv_cell_cfg);
  }

  // Create a temporary struct that will be fed to the function alloc_resources().
  odu::cell_group_config cell_group_cfg;
  cell_group_cfg.cells.emplace(0, cell_config_dedicated{.serv_cell_cfg = serv_cell_cfg});
  const bool alloc_outcome = pucch_res_mgr.alloc_resources(cell_group_cfg);
  if (not alloc_outcome) {
    return false;
  }
  // Copy the serv_cell_cfg configuration in cell_group_cfg the input serv_cell_cfg.
  serv_cell_cfg = cell_group_cfg.cells[0].serv_cell_cfg;
  return true;
}

void pucch_res_builder_test_helper::init_pucch_res_mgr(const serving_cell_config& base_ue_cfg)
{
  ran_cell_config cell_cfg = generate_ran_cell_config(required_info.value().init_ul_bwp,
                                                      required_info.value().tdd_ul_dl_cfg_common,
                                                      base_ue_cfg,
                                                      required_info.value().pucch_cfg);
  pucch_res_mgr.add_cell(to_du_cell_index(0), cell_cfg);
}
