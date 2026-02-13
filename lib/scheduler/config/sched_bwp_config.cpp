/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "sched_bwp_config.h"
#include "../support/pdcch/pdcch_mapping.h"

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

sched_bwp_config::sched_bwp_config(pci_t                         pci,
                                   bwp_id_t                      bwp_id_,
                                   const bwp_downlink_common&    base_dl_bwp_cmn_,
                                   const bwp_downlink_dedicated* base_dl_bwp_) :
  bwpid(bwp_id_),
  base_dl_bwp_cmn(base_dl_bwp_cmn_),
  base_dl_bwp_ded(base_dl_bwp_ != nullptr ? std::optional<bwp_downlink_dedicated>{*base_dl_bwp_}
                                          : std::optional<bwp_downlink_dedicated>{})
{
  if (base_dl_bwp_cmn.pdcch_common.coreset0.has_value()) {
    cs_list.emplace(to_coreset_id(0),
                    sched_coreset_config(pci, base_dl_bwp_cmn, *base_dl_bwp_cmn.pdcch_common.coreset0));
  }
  if (base_dl_bwp_cmn.pdcch_common.common_coreset.has_value()) {
    cs_list.emplace(base_dl_bwp_cmn.pdcch_common.common_coreset->get_id(),
                    sched_coreset_config(pci, base_dl_bwp_cmn, *base_dl_bwp_cmn.pdcch_common.common_coreset));
  }
  if (base_dl_bwp_ded.has_value() and base_dl_bwp_ded->pdcch_cfg.has_value()) {
    for (const auto& cs_cfg : base_dl_bwp_ded->pdcch_cfg->coresets) {
      cs_list.emplace(cs_cfg.get_id(), sched_coreset_config(pci, base_dl_bwp_cmn, cs_cfg));
    }
  }
}
