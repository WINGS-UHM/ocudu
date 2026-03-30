// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "sched_bwp_config.h"
#include "../support/pdcch/pdcch_mapping.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"

using namespace ocudu;

sched_coreset_config::sched_coreset_config(pci_t                        pci,
                                           const bwp_configuration&     bwp_cfg,
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
      al_crb_lists.push_back(pdcch_helper::cce_to_prb_mapping(bwp_cfg, cs_cfg, pci, aggr_lvl, start_ncce));

      // Convert PRBs to CRBs.
      for (uint16_t& prb_idx : al_crb_lists.back()) {
        prb_idx = prb_to_crb(bwp_cfg.crbs, prb_idx);
      }
    }
  }
}

sched_bwp_config::sched_bwp_config(const ran_cell_config& ran_cfg, bwp_id_t bwp_id_) :
  bwpid(bwp_id_), base_dl_bwp_cmn(ran_cfg.dl_cfg_common.init_dl_bwp), res(make_cell_bwp_res_config(ran_cfg))
{
  // Fill sched_coreset_config list with unique entries.
  if (base_dl_bwp_cmn.pdcch_common.coreset0.has_value()) {
    cs_list.emplace_back(ran_cfg.pci, base_dl_bwp_cmn.generic_params, *base_dl_bwp_cmn.pdcch_common.coreset0);
  }
  if (base_dl_bwp_cmn.pdcch_common.common_coreset.has_value()) {
    cs_list.emplace_back(ran_cfg.pci, base_dl_bwp_cmn.generic_params, *base_dl_bwp_cmn.pdcch_common.common_coreset);
  }
  for (auto& ded_pdcch : res.dl.ded_pdcchs) {
    for (const auto& cs : ded_pdcch.coresets) {
      if (std::find(cs_list.begin(), cs_list.end(), cs) == cs_list.end()) {
        cs_list.emplace_back(ran_cfg.pci, base_dl_bwp_cmn.generic_params, cs);
      }
    }
  }
}
