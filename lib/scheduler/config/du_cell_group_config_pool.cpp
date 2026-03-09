// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "du_cell_group_config_pool.h"
#include "cell_configuration.h"
#include "ocudu/ran/bwp/bwp_id.h"
#include "ocudu/scheduler/config/ue_bwp_config.h"
#include "ocudu/scheduler/scheduler_configurator.h"

using namespace ocudu;

du_cell_config_pool::du_cell_config_pool(const scheduler_expert_config&                  sched_cfg_,
                                         const sched_cell_configuration_request_message& req) :
  cell_cfg_inst(sched_cfg_, req),
  init_dl_bwp(cell_cfg_inst.params.dl_cfg_common.init_dl_bwp),
  init_ul_bwp(cell_cfg_inst.params.ul_cfg_common.init_ul_bwp)
{
}

ue_cell_config_ptr du_cell_config_pool::update_ue(const ue_cell_config& ue_cell)
{
  ue_cell_res_config ret;
  ret.cell_index = ue_cell.serv_cell_cfg.cell_index;
  if (ue_cell.serv_cell_cfg.ul_config.has_value() and
      ue_cell.serv_cell_cfg.ul_config.value().pusch_serv_cell_cfg.has_value()) {
    ret.pusch_serv_cell_cfg.emplace(
        pusch_serv_cell_pool.create(ue_cell.serv_cell_cfg.ul_config.value().pusch_serv_cell_cfg.value()));
  }
  if (ue_cell.serv_cell_cfg.pdsch_serv_cell_cfg.has_value()) {
    ret.pdsch_serv_cell_cfg = pdsch_serv_cell_pool.create(ue_cell.serv_cell_cfg.pdsch_serv_cell_cfg.value());
  }
  if (ue_cell.serv_cell_cfg.csi_meas_cfg.has_value()) {
    ret.csi_meas_cfg = csi_meas_config_pool.create(ue_cell.serv_cell_cfg.csi_meas_cfg.value());
  }
  ret.tag_id = ue_cell.serv_cell_cfg.tag_id;

  for (unsigned bwp_id = 0, nof_bwps = ue_cell.bwps.size(); bwp_id != nof_bwps; ++bwp_id) {
    if (bwp_id == 0) {
      // Initial BWP.
      add_bwp(ret,
              to_bwp_id(bwp_id),
              init_dl_bwp,
              ue_cell.serv_cell_cfg.init_dl_bwp,
              &init_ul_bwp,
              ue_cell.serv_cell_cfg.ul_config.has_value() ? &ue_cell.serv_cell_cfg.ul_config->init_ul_bwp : nullptr,
              ue_cell.bwps[bwp_id]);
    } else {
      // Non-initial BWPs.
      const auto& bwp = ue_cell.serv_cell_cfg.dl_bwps[bwp_id];
      add_bwp(ret, bwp.bwp_id, bwp.bwp_dl_common, bwp.bwp_dl_ded, nullptr, nullptr, ue_cell.bwps[bwp_id]);
    }
  }

  return cell_cfg_pool.create(ret);
}

void du_cell_config_pool::add_bwp(ue_cell_res_config&           out,
                                  bwp_id_t                      bwp_id,
                                  const bwp_downlink_common&    dl_bwp_common,
                                  const bwp_downlink_dedicated& dl_bwp_ded,
                                  const bwp_uplink_common*      ul_bwp_common,
                                  const bwp_uplink_dedicated*   ul_bwp_ded,
                                  const ue_bwp_config&          ue_bwp_cfg)
{
  // Create BWP config.
  bwp_config bwp_cfg;
  bwp_cfg.bwp_id = bwp_id;
  // BWP DL Common
  bwp_cfg.dl_common = bwp_dl_common_config_pool.create(dl_bwp_common);
  if (dl_bwp_common.pdcch_common.coreset0.has_value()) {
    const auto& coreset0 = dl_bwp_common.pdcch_common.coreset0.value();
    bwp_cfg.coresets.emplace(coreset0.get_id(), coreset_config_pool.create(coreset0));
    out.coresets.emplace(coreset0.get_id(), bwp_cfg.coresets[coreset0.get_id()]);
  }
  if (dl_bwp_common.pdcch_common.common_coreset.has_value()) {
    const auto& common_coreset = dl_bwp_common.pdcch_common.common_coreset.value();
    bwp_cfg.coresets.emplace(common_coreset.get_id(), coreset_config_pool.create(common_coreset));
    out.coresets.emplace(common_coreset.get_id(), bwp_cfg.coresets[common_coreset.get_id()]);
  }
  for (const search_space_configuration& ss : dl_bwp_common.pdcch_common.search_spaces) {
    bwp_cfg.search_spaces.emplace(ss.get_id(), ss_config_pool.create(ss));
  }

  // BWP DL Dedicated
  bwp_cfg.dl_ded = bwp_dl_ded_config_pool.create(dl_bwp_ded);
  if (dl_bwp_ded.pdcch_cfg.has_value()) {
    for (const coreset_configuration& cs : dl_bwp_ded.pdcch_cfg->coresets) {
      // TS 38.331, "PDCCH-Config" - In case network reconfigures control resource set with the same
      // ControlResourceSetId as used for commonControlResourceSet configured via PDCCH-ConfigCommon,
      // the configuration from PDCCH-Config always takes precedence and should not be updated by the UE based on
      // servingCellConfigCommon.
      bwp_cfg.coresets.emplace(cs.get_id(), coreset_config_pool.create(cs));
      out.coresets.emplace(cs.get_id(), bwp_cfg.coresets[cs.get_id()]);
    }
    for (const search_space_configuration& ss : dl_bwp_ded.pdcch_cfg->search_spaces) {
      bwp_cfg.search_spaces.emplace(ss.get_id(), ss_config_pool.create(ss));
      out.search_spaces.emplace(ss.get_id(), bwp_cfg.search_spaces[ss.get_id()]);
    }
  }

  // BWP UL Common
  if (ul_bwp_common != nullptr) {
    bwp_cfg.ul_common = bwp_ul_common_config_pool.create(*ul_bwp_common);
  }

  // BWP UL Dedicated
  if (ul_bwp_ded != nullptr) {
    bwp_cfg.ul_ded = *ul_bwp_ded;
  }

  bwp_cfg.bwp = ue_bwp_cfg;
  out.bwps.emplace(bwp_id, bwp_config_pool.create(bwp_cfg));
}

// class du_cell_group_config_pool

cell_configuration& du_cell_group_config_pool::add_cell(const scheduler_expert_config&                  expert_cfg,
                                                        const sched_cell_configuration_request_message& cell_cfg_req)
{
  ocudu_assert(not cells.contains(cell_cfg_req.cell_index), "Cell already exists");
  return cells.emplace(cell_cfg_req.cell_index, std::make_unique<du_cell_config_pool>(expert_cfg, cell_cfg_req))
      ->cell_cfg();
}

void du_cell_group_config_pool::rem_cell(du_cell_index_t cell_index)
{
  // Note: This function assumes that all the UEs have been removed from this cell.
  cells.erase(cell_index);
}

ue_creation_params du_cell_group_config_pool::add_ue(const sched_ue_creation_request_message& ue_creation_req)
{
  // Create logical channel config.
  auto lc_ch_list =
      lc_ch_pool.create(ue_creation_req.cfg.lc_config_list.has_value() ? ue_creation_req.cfg.lc_config_list.value()
                                                                       : std::vector<logical_channel_config>{});

  // Create UE dedicated cell configs.
  slotted_id_vector<du_cell_index_t, ue_cell_config_ptr> cell_cfgs;
  if (ue_creation_req.cfg.cells.has_value()) {
    for (const auto& cell : ue_creation_req.cfg.cells.value()) {
      cell_cfgs.emplace(cell.serv_cell_cfg.cell_index, cells[cell.serv_cell_cfg.cell_index]->update_ue(cell));
    }
  }

  return ue_creation_params{ue_creation_req.cfg, lc_ch_list, cell_cfgs};
}

ue_reconfig_params du_cell_group_config_pool::reconf_ue(const sched_ue_reconfiguration_message& ue_reconf_req)
{
  std::optional<logical_channel_config_list_ptr> lc_ch_list;
  if (ue_reconf_req.cfg.lc_config_list.has_value()) {
    lc_ch_list = lc_ch_pool.create(ue_reconf_req.cfg.lc_config_list.value());
  }

  // Create UE dedicated cell configs.
  slotted_id_vector<du_cell_index_t, ue_cell_config_ptr> cell_cfgs;
  if (ue_reconf_req.cfg.cells.has_value()) {
    for (const auto& cell : ue_reconf_req.cfg.cells.value()) {
      cell_cfgs.emplace(cell.serv_cell_cfg.cell_index, cells[cell.serv_cell_cfg.cell_index]->update_ue(cell));
    }
  }

  return ue_reconfig_params{ue_reconf_req.cfg, lc_ch_list, cell_cfgs};
}
