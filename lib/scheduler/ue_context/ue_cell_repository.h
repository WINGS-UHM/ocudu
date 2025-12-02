/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ue_cell.h"
#include "ocudu/adt/flat_map.h"
#include "ocudu/ran/du_types.h"

namespace ocudu {

/// Container that stores all the UEs that are configured in a given cell.
class ue_cell_repository
{
  using ue_list = slotted_id_table<du_ue_index_t, ue_cell, MAX_NOF_DU_UES>;

public:
  using value_type     = ue_list::value_type;
  using iterator       = ue_list::iterator;
  using const_iterator = ue_list::const_iterator;

  ue_cell_repository(du_cell_index_t cell_idx, ocudulog::basic_logger& logger_);

  du_cell_index_t cell_index() const { return cell_idx; }
  bool            contains(du_ue_index_t ue_index) const { return ues.contains(ue_index); }
  bool     contains(rnti_t rnti) const { return rnti_to_ue_index_lookup.find(rnti) != rnti_to_ue_index_lookup.end(); }
  size_t   size() const { return ues.size(); }
  bool     empty() const { return ues.empty(); }
  iterator begin() { return ues.begin(); }
  iterator end() { return ues.end(); }
  const_iterator begin() const { return ues.begin(); }
  const_iterator end() const { return ues.end(); }

  ue_cell&       operator[](du_ue_index_t ue_index) { return ues[ue_index]; }
  const ue_cell& operator[](du_ue_index_t ue_index) const { return ues[ue_index]; }
  ue_cell*       find(du_ue_index_t ue_index) { return ues.contains(ue_index) ? &ues[ue_index] : nullptr; }
  const ue_cell* find(du_ue_index_t ue_index) const { return ues.contains(ue_index) ? &ues[ue_index] : nullptr; }
  ue_cell*       find_by_rnti(rnti_t rnti)
  {
    auto it = rnti_to_ue_index_lookup.find(rnti);
    return it != rnti_to_ue_index_lookup.end() ? &ues[it->second] : nullptr;
  }
  const ue_cell* find_by_rnti(rnti_t rnti) const
  {
    auto it = rnti_to_ue_index_lookup.find(rnti);
    return it != rnti_to_ue_index_lookup.end() ? &ues[it->second] : nullptr;
  }

private:
  friend class ue_repository;

  /// Add a new UE to the UE cell repository.
  ue_cell& add_ue(const ue_configuration&   ue_cfg,
                  ue_cell_index_t           ue_cell_index,
                  cell_harq_manager&        harq_pool,
                  ue_drx_controller&        drx,
                  std::optional<slot_point> msg3_slot_rx);

  void rem_ue(du_ue_index_t ue_index);

  const du_cell_index_t   cell_idx;
  ocudulog::basic_logger& logger;

  // List of UEs in the cell.
  ue_list ues;

  // Mapping of RNTIs to UE indexes.
  flat_map<rnti_t, du_ue_index_t> rnti_to_ue_index_lookup;

  // UE carrier components.
  slotted_id_table<du_ue_index_t, ue_channel_state_manager, MAX_NOF_DU_UES>      channel_states;
  slotted_id_table<du_ue_index_t, ue_link_adaptation_controller, MAX_NOF_DU_UES> ue_mcs_calculators;
  slotted_id_table<du_ue_index_t, pusch_power_controller, MAX_NOF_DU_UES>        pusch_pwr_controllers;
  slotted_id_table<du_ue_index_t, pucch_power_controller, MAX_NOF_DU_UES>        pucch_pwr_controllers;
};

} // namespace ocudu
