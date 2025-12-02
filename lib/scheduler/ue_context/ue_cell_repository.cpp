/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ue_cell_repository.h"

using namespace ocudu;

ue_cell_repository::ue_cell_repository(du_cell_index_t cell_idx_, ocudulog::basic_logger& logger_) :
  cell_idx(cell_idx_), logger(logger_)
{
  rnti_to_ue_index_lookup.reserve(MAX_NOF_DU_UES);
}

ue_cell& ue_cell_repository::add_ue(const ue_configuration&   ue_cfg,
                                    ue_cell_index_t           ue_cell_index,
                                    cell_harq_manager&        harq_pool,
                                    ue_drx_controller&        drx,
                                    std::optional<slot_point> msg3_slot_rx)
{
  ocudu_assert(not ues.contains(ue_cfg.ue_index), "UE with duplicate index being added to the cell UE repository");
  auto& ue_cell_cfg = ue_cfg.ue_cell_cfg(ue_cell_index);

  // Create UE cell components.
  channel_states.emplace(ue_cfg.ue_index, ue_cell_cfg.cell_cfg_common.expert_cfg.ue, ue_cell_cfg.get_nof_dl_ports());
  ue_mcs_calculators.emplace(ue_cfg.ue_index, ue_cell_cfg.cell_cfg_common, channel_states[ue_cfg.ue_index]);
  pusch_pwr_controllers.emplace(ue_cfg.ue_index, ue_cell_cfg, channel_states[ue_cfg.ue_index]);
  pucch_pwr_controllers.emplace(ue_cfg.ue_index, ue_cell_cfg);

  // Add UE in the repository.
  ues.emplace(ue_cfg.ue_index,
              ue_cfg.ue_index,
              ue_cfg.crnti,
              ue_cell_index,
              ue_cell_cfg,
              harq_pool,
              ue_shared_context{drx},
              ue_cell_components{&channel_states[ue_cfg.ue_index],
                                 &ue_mcs_calculators[ue_cfg.ue_index],
                                 &pusch_pwr_controllers[ue_cfg.ue_index],
                                 &pucch_pwr_controllers[ue_cfg.ue_index]},
              msg3_slot_rx);
  auto res = rnti_to_ue_index_lookup.insert(std::make_pair(ue_cfg.crnti, ue_cfg.ue_index));
  ocudu_assert(res.second, "UE with duplicate RNTI being added to the cell UE repository");
  return ues[ue_cfg.ue_index];
}

void ue_cell_repository::rem_ue(du_ue_index_t ue_index)
{
  if (not ues.contains(ue_index)) {
    logger.error("ue={} : UE not found in the cell UE repository", fmt::underlying(ue_index));
  }
  const ue_cell&      u      = ues[ue_index];
  const rnti_t        crnti  = u.rnti();
  const du_ue_index_t ue_idx = u.ue_index;

  // Remove UE from lookup.
  auto it = rnti_to_ue_index_lookup.find(crnti);
  if (it != rnti_to_ue_index_lookup.end()) {
    rnti_to_ue_index_lookup.erase(it);
  } else {
    logger.error("ue={} rnti={}: UE with provided c-rnti not found in RNTI-to-UE-index lookup table.",
                 fmt::underlying(ue_idx),
                 crnti);
  }

  pucch_pwr_controllers.erase(ue_idx);
  pusch_pwr_controllers.erase(ue_idx);
  ue_mcs_calculators.erase(ue_idx);
  channel_states.erase(ue_idx);

  // Take the ue cell from the repository.
  ues.erase(ue_idx);
}
