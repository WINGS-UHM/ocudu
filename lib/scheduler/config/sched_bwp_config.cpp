// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "sched_bwp_config.h"
#include "../support/pdcch/pdcch_mapping.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"

using namespace ocudu;

sched_coreset_config::sched_coreset_config(pci_t                        pci,
                                           const bwp_downlink_common&   dl_bwp_cmn,
                                           const coreset_configuration& cs_cfg) :
  cfg_ptr(&cs_cfg)
{
  for (unsigned al_idx = 0; al_idx != NOF_AGGREGATION_LEVELS; ++al_idx) {
    const aggregation_level      aggr_lvl     = aggregation_index_to_level(al_idx);
    std::vector<crb_index_list>& al_crb_lists = ncce_crbs[al_idx];

    // Get PRBs for each candidate.
    // Note: Start CCE is always a multiple of L.
    unsigned L = to_nof_cces(aggr_lvl);
    for (unsigned start_ncce = 0, nof_cces = cs_cfg.get_nof_cces(); start_ncce < nof_cces; start_ncce += L) {
      al_crb_lists.push_back(
          pdcch_helper::cce_to_prb_mapping(dl_bwp_cmn.generic_params, cs_cfg, pci, aggr_lvl, start_ncce));

      // Convert PRBs to CRBs.
      for (uint16_t& prb_idx : al_crb_lists.back()) {
        prb_idx = prb_to_crb(dl_bwp_cmn.generic_params.crbs, prb_idx);
      }
    }
  }
}

static bwp_downlink_dedicated get_bwp_ded(const ran_cell_config& ran_cfg, bwp_id_t bwp_id)
{
  ocudu_assert(bwp_id == to_bwp_id(0), "non init BWP not supported");
  const auto init_dl_bwp = config_helpers::make_default_ue_cell_config(ran_cfg).serv_cell_cfg.init_dl_bwp;
  return bwp_downlink_dedicated{
      .pdcch_cfg = ran_cfg.init_bwp.pdcch_cfg, .pdsch_cfg = init_dl_bwp.pdsch_cfg, .rlm_cfg = init_dl_bwp.rlm_cfg};
}

sched_bwp_config::sched_bwp_config(const ran_cell_config& ran_cfg, bwp_id_t bwp_id_) :
  bwpid(bwp_id_),
  base_dl_bwp_cmn(ran_cfg.dl_cfg_common.init_dl_bwp),
  base_dl_bwp_ded(get_bwp_ded(ran_cfg, bwp_id())),
  ul_res(make_cell_bwp_res_config(ran_cfg).ul)
{
  if (base_dl_bwp_cmn.pdcch_common.coreset0.has_value()) {
    cs_list.emplace(to_coreset_id(0),
                    sched_coreset_config(ran_cfg.pci, base_dl_bwp_cmn, *base_dl_bwp_cmn.pdcch_common.coreset0));
  }
  if (base_dl_bwp_cmn.pdcch_common.common_coreset.has_value()) {
    cs_list.emplace(base_dl_bwp_cmn.pdcch_common.common_coreset->get_id(),
                    sched_coreset_config(ran_cfg.pci, base_dl_bwp_cmn, *base_dl_bwp_cmn.pdcch_common.common_coreset));
  }
  if (base_dl_bwp_ded.has_value() and base_dl_bwp_ded->pdcch_cfg.has_value()) {
    for (const auto& cs_cfg : base_dl_bwp_ded->pdcch_cfg->coresets) {
      cs_list.emplace(cs_cfg.get_id(), sched_coreset_config(ran_cfg.pci, base_dl_bwp_cmn, cs_cfg));
    }
  }
}
