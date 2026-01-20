/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_srs_aperiodic_res_mng.h"
#include "du_srs_manager_helpers.h"
#include "du_ue_resource_config.h"
#include "ocudu/du/du_cell_config_helpers.h"
#include "ocudu/ran/srs/srs_configuration.h"
#include "ocudu/ran/srs/srs_constants.h"
#include "ocudu/scheduler/config/pusch_td_resource_indices.h"

using namespace ocudu;
using namespace odu;

// Helper that computes the slot offsets that can be used for the activation of the SRS resource sets (ref. to
// Section 6.2.1, TS 38.214).
// These offsets are chosen:
// - So that the scheduler can trigger the aperiodic SRS transmission for the UE with both DCI 0_1 and 1_1; this way,
// the scheduler can take advantage of both DL and UL allocation to scheduler an SRS.
// - To be greater than the maximum k2, so that the SRS will be allocated before the PUSCH.
static std::vector<unsigned> compute_slot_offsets(const du_cell_config& cell_cfg)
{
  const auto& dl_data_to_ul_ack =
      cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.pucch_cfg.value().dl_data_to_ul_ack;

  std::vector<static_vector<unsigned, pusch_constants::MAX_NOF_PUSCH_TD_RES_ALLOCS>> pusch_td_list_per_slot =
      get_fairly_distributed_pusch_td_resource_indices(cell_cfg.scs_common,
                                                       cell_cfg.tdd_ul_dl_cfg_common,
                                                       cell_cfg.ul_cfg_common.init_ul_bwp.pusch_cfg_common.value(),
                                                       dl_data_to_ul_ack);

  const auto& pusch_td_alloc_list = cell_cfg.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;

  // Find the maximum k2 used for UL scheduling.
  unsigned max_used_k2 = 0;
  for (const auto& dl_td_res_vec : pusch_td_list_per_slot) {
    for (const auto td_res_idx : dl_td_res_vec) {
      max_used_k2 = std::max(max_used_k2, static_cast<unsigned>(pusch_td_alloc_list[td_res_idx].k2));
    }
  }

  // [Implementation-defined] Take the max between min_k1 and k2; this way we ensure that:
  // - The slot offset is greater than the min_k1; if we assume the UE's SRS latency requirement is the same as for the
  // PUCCH one, a slot offset > min_k1 ensures the DCI-to-SRS latency constraint is met. Note, the TS sets some min
  // constraints for DCI-to-SRS (as per Section 6.2.1, TS 38.214); however, some test tools work better with looser
  // latency constraints.
  // - If at a given PDCCH slot we trigger an SRS with a given slot_offset, there won't be any PUSCH yet allocated at
  // PDCCH slot + slot_offset.
  const auto* min_k1_it = std::min_element(dl_data_to_ul_ack.begin(), dl_data_to_ul_ack.end());
  ocudu_assert(min_k1_it != dl_data_to_ul_ack.end(), "Min k1 must exist");
  const unsigned min_k1                  = *min_k1_it;
  const int      slot_offset_lower_bound = static_cast<int>(std::max(min_k1, max_used_k2));

  // FDD Case.
  if (not cell_cfg.tdd_ul_dl_cfg_common.has_value()) {
    // Return 1 slot offsets value, to ensure that the SRS is allocated before the PUSCH.
    return {static_cast<unsigned>(slot_offset_lower_bound + 1)};
  }

  const auto& tdd_cfg = cell_cfg.tdd_ul_dl_cfg_common.value();

  // TDD Case.

  // Get the target UL slot indices, i.e., the slots in which we aim at allocating the SRS. If present, we consider the
  // special slots, else the first UL slot.
  std::vector<unsigned> target_ul_slots_idx;
  if (tdd_cfg.pattern1.nof_ul_symbols != 0) {
    target_ul_slots_idx.emplace_back(tdd_cfg.pattern1.nof_dl_slots);
  }
  if (tdd_cfg.pattern2.has_value() and tdd_cfg.pattern2.value().nof_ul_slots != 0 and
      tdd_cfg.pattern2.value().nof_dl_symbols != 0) {
    target_ul_slots_idx.emplace_back(tdd_cfg.pattern1.dl_ul_tx_period_nof_slots +
                                     tdd_cfg.pattern2.value().nof_dl_slots);
  }
  if (target_ul_slots_idx.empty()) {
    target_ul_slots_idx.emplace_back(tdd_cfg.pattern1.dl_ul_tx_period_nof_slots - tdd_cfg.pattern1.nof_ul_slots);
  }

  const unsigned tdd_period_slots = nof_slots_per_tdd_period(tdd_cfg);

  // Tuple { DL slot index, slot_offset, is_for_DL_DCI }.
  // DL slot index = slot index of the DL slot, from 0 to TDD period in slots - 1.
  // slot_offset = parameter for SRS config or \c slotOffset, as per \c SRS-ResourceSet, \c SRS-Config, TS 38.331.
  // is_for_DL_DCI = true if this offset is used to allocate SRS through DL DCI, false for UL DCI.
  using dl_sl_idx_sl_offset_is_dl_tuple = std::tuple<uint8_t, uint8_t, bool>;

  // Find the candidate UL slot offsets, i.e., the slot offsets that can be used to trigger aperiodic SRS with UL DCIs.
  // These UL slot offsets only exist for DL slots that has at least a k2 value (otherwise, the DL slot can't be used
  // for UL DCIs).
  std::vector<dl_sl_idx_sl_offset_is_dl_tuple> candidate_slot_offsets;
  for (unsigned dl_sl_idx = 0, sz = pusch_td_list_per_slot.size(); dl_sl_idx != sz; ++dl_sl_idx) {
    // Skip UL slots.
    if (pusch_td_list_per_slot[dl_sl_idx].empty()) {
      continue;
    }
    // Save the viable offsets that map the DL slots suitable for UL DCI to the candidate SRS UL slots.
    for (const auto target_slot_idx : target_ul_slots_idx) {
      // Consider the offsets that spans over several TDD periods, otherwise the constraint
      // sl_offset > slot_offset_lower_bound might not be met.
      // The maximum period multiplier 3 is the for a 4-slot TDD period to cover the worst case for the constraint
      // sl_offset > SCHEDULER_MAX_K2.
      for (unsigned period_multiplier = 0; period_multiplier != 4; ++period_multiplier) {
        const auto sl_offset =
            static_cast<int>(target_slot_idx + period_multiplier * tdd_period_slots) - static_cast<int>(dl_sl_idx);
        // The constraint offsets > slot_offset_lower_bound ensures the SRS is always allocated before a PUSCH.
        if (sl_offset > slot_offset_lower_bound and sl_offset <= static_cast<int>(srs_constants::MAX_SRS_SLOT_OFFSET)) {
          candidate_slot_offsets.emplace_back(dl_sl_idx, sl_offset, /* is_dl */ false);
        }
      }
    }
  }

  // Find the candidate DL slot offsets, i.e., the slot offsets that can be used to trigger aperiodic SRS with DL DCIs.
  for (unsigned dl_sl_idx = 0, sz = tdd_period_slots; dl_sl_idx != sz; ++dl_sl_idx) {
    if (not has_active_tdd_dl_symbols(tdd_cfg, dl_sl_idx)) {
      continue;
    }
    // Save the viable offsets that map the DL slots suitable for DL DCI to the candidate SRS UL slots.
    for (const auto target_slot_idx : target_ul_slots_idx) {
      // Consider the offsets that spans over several TDD periods, otherwise the constraint
      // sl_offset > slot_offset_lower_bound might not be met.
      // The maximum period multiplier 3 is the for a 4-slot TDD period to cover the worst case for the constraint
      // sl_offset > SCHEDULER_MAX_K2.
      for (unsigned period_multiplier = 0; period_multiplier != 4; ++period_multiplier) {
        const auto sl_offset =
            static_cast<int>(target_slot_idx + period_multiplier * tdd_period_slots) - static_cast<int>(dl_sl_idx);
        // The constraint offsets > slot_offset_lower_bound ensures the SRS is always allocated before a PUSCH.
        if (sl_offset > slot_offset_lower_bound and sl_offset <= static_cast<int>(srs_constants::MAX_SRS_SLOT_OFFSET)) {
          candidate_slot_offsets.emplace_back(dl_sl_idx, sl_offset, /* is_dl */ true);
        }
      }
    }
  }

  std::vector<dl_sl_idx_sl_offset_is_dl_tuple> optimal_tuples;
  auto weight_function = [&optimal_tuples, &tdd_cfg, &target_ul_slots_idx, tdd_period_slots](
                             const dl_sl_idx_sl_offset_is_dl_tuple& offset) {
    // The logic of this function can be summarized as follows:
    // - Provide at least 1 UL slot offset and 1 DL slot offset.
    // - If a third one is required, then provide an extra UL(DL) slot offset for UL(DL)-heavy configuration.
    // - If possible, provide a slot offset that reaches the UL target slot from different DL slot (if another slot
    // offset exits).
    // - Don't consider an offset that has already been chosen, even if the metric is good; the goal is to provide a
    // list of offsets that are different, so the scheduler has more option when it comes to find a possible DL slot for
    // the DCI that triggers the SRS transmission.
    const bool         ul_heavy = nof_dl_slots_per_tdd_period(tdd_cfg) > nof_full_ul_slots_per_tdd_period(tdd_cfg);
    constexpr unsigned exclude_element_penalty = 1000;
    // Always start the search from an UL slot offset, first, as in DL-heavy the search space for UL slot offset is
    // smaller than for DL slot offsets.
    if (optimal_tuples.empty()) {
      const unsigned is_dl_penalty = std::get<2>(offset) ? exclude_element_penalty : 0U;
      return std::get<1>(offset) + is_dl_penalty;
    }
    // Penalty to force the choice of a slot offset starting from a different DL slot index.
    const unsigned existing_dl_slot_penalty = std::find_if(optimal_tuples.begin(),
                                                           optimal_tuples.end(),
                                                           [offset](const dl_sl_idx_sl_offset_is_dl_tuple& tuple) {
                                                             return std::get<0>(tuple) == std::get<0>(offset);
                                                           }) != optimal_tuples.end()
                                                  ? 2 * srs_constants::MAX_SRS_SLOT_OFFSET
                                                  : 0U;
    // Penalty to force the choice of a different slot offset.
    const unsigned existing_offset_penalty = std::find_if(optimal_tuples.begin(),
                                                          optimal_tuples.end(),
                                                          [offset](const dl_sl_idx_sl_offset_is_dl_tuple& tuple) {
                                                            return std::get<1>(tuple) == std::get<1>(offset);
                                                          }) != optimal_tuples.end()
                                                 ? exclude_element_penalty
                                                 : 0U;
    // Penalty to force the choice of a different UL slot target (only needed if the TDD patterns has 2 special slots).
    const unsigned same_target_ul_slot_penalty =
        target_ul_slots_idx.size() == 2 and
                std::find_if(optimal_tuples.begin(),
                             optimal_tuples.end(),
                             [offset, tdd_period_slots](const dl_sl_idx_sl_offset_is_dl_tuple& tuple) {
                               return (std::get<0>(tuple) + std::get<1>(tuple)) % tdd_period_slots ==
                                      (std::get<0>(offset) + std::get<1>(offset)) % tdd_period_slots;
                             }) != optimal_tuples.end()
            ? srs_constants::MAX_SRS_SLOT_OFFSET
            : 0U;
    // Choose a DL slot offset as second value.
    if (optimal_tuples.size() == 1) {
      const unsigned is_ul_penalty = std::get<2>(offset) ? 0U : exclude_element_penalty;
      const unsigned weight = std::get<1>(offset) + existing_dl_slot_penalty + existing_offset_penalty + is_ul_penalty +
                              same_target_ul_slot_penalty;
      return weight;
    }
    // Choose a DL/UL slot offset as second value, depending on DL-UL heavy config.
    if (optimal_tuples.size() == 2) {
      // If UL heavy then we add a penalty for DL slots offsets. If DL heavy, we add a penalty for UL.
      const unsigned dl_ul_penalty = exclude_element_penalty * static_cast<unsigned>(ul_heavy != std::get<2>(offset));
      const unsigned weight = std::get<1>(offset) + dl_ul_penalty + existing_dl_slot_penalty + existing_offset_penalty +
                              same_target_ul_slot_penalty;
      return weight;
    }

    return 0U;
  };

  // We consider max 3 slots offsets, as many as the possible activation states defined in Table 7.3.1.1.2-24,
  // TS 38.212. NOTE, if the TDD configuration only has 2 DL slot, then the 3rd slot offset wouldn't be used.
  const unsigned nof_slot_offset = std::min(3U, nof_dl_slots_per_tdd_period(tdd_cfg));
  for (unsigned n = 0; n != nof_slot_offset; ++n) {
    auto min_it = std::min_element(
        candidate_slot_offsets.begin(),
        candidate_slot_offsets.end(),
        [&weight_function](const dl_sl_idx_sl_offset_is_dl_tuple& lhs, const dl_sl_idx_sl_offset_is_dl_tuple& rhs) {
          return weight_function(lhs) < weight_function(rhs);
        });
    ocudu_assert(min_it != optimal_tuples.end(), "");
    optimal_tuples.emplace_back(*min_it);
  }

  std::vector<unsigned> slot_offsets;
  for (const auto& tuple : optimal_tuples) {
    slot_offsets.emplace_back(std::get<1>(tuple));
  }
  std::sort(slot_offsets.begin(), slot_offsets.end());
  return slot_offsets;
}

du_srs_aperiodic_res_mng::cell_context::cell_context(const du_cell_config& cfg) :
  cell_cfg(cfg),
  tdd_ul_dl_cfg_common(cfg.tdd_ul_dl_cfg_common),
  default_srs_cfg(du_srs_mng_details::build_default_srs_cfg<false>(cfg))
{
}

du_srs_aperiodic_res_mng::du_srs_aperiodic_res_mng(span<const du_cell_config> cell_cfg_list_) :
  cells(cell_cfg_list_.begin(), cell_cfg_list_.end())
{
  for (auto& cell : cells) {
    ocudu_assert(cell.cell_cfg.ue_ded_serv_cell_cfg.ul_config.has_value() and
                     cell.cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.has_value(),
                 "DU cell config is not valid");

    ocudu_assert(cell.cell_cfg.srs_cfg.srs_type_enabled != srs_type::periodic,
                 "Request to build aperiodic SRS configuration, but periodic parameters have been provided");

    if (cell.cell_cfg.srs_cfg.srs_type_enabled == srs_type::disabled) {
      continue;
    }

    const auto& cell_cfg = cell.cell_cfg;

    // If the C_SRS is not set as an input parameter, then we compute C_SRS so that the SRS uses the maximum allowed
    // number of RBs and is located at the center of the UL BWP.
    if (cell_cfg.srs_cfg.c_srs.has_value()) {
      cell.srs_common_params.c_srs      = cell_cfg.srs_cfg.c_srs.value();
      cell.srs_common_params.freq_shift = cell_cfg.srs_cfg.freq_domain_shift.value();
    } else {
      const std::optional<unsigned> c_srs =
          du_srs_mng_details::compute_c_srs(cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length());
      ocudu_assert(c_srs.has_value(), "SRS parameters didn't provide a valid C_SRS value");
      cell.srs_common_params.c_srs = c_srs.value();
      // When computed automatically, \c freqDomainShift is set so that the SRS is placed at the center of the UL BWP.
      // As per TS 38.211, Section 6.4.1.4.3, if \f$n_{shift} >= BWP_RB_start\f$, the reference point for the SRS
      // subcarriers is the CRB idx 0, else it's the BWP_RB_start; in here, we implicitly assume \f$n_{shift} >=
      // BWP_RB_start\f$.
      cell.srs_common_params.freq_shift =
          du_srs_mng_details::compute_srs_rb_start(c_srs.value(),
                                                   cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length()) +
          cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.start();
    }

    cell.srs_common_params.p0 = cell_cfg.srs_cfg.p0;

    // TODO: evaluate whether we need to consider the case of multiple cells.
    // NOTE: If there is pattern2, then we expect pattern 2 to have the same number of symbols in the special slot as
    // pattern1.
    const bool use_special_slot_only = cell_cfg.tdd_ul_dl_cfg_common.has_value() and
                                       (cell_cfg.tdd_ul_dl_cfg_common.value().pattern1.nof_ul_symbols != 0);
    cell.cell_srs_res_list = generate_cell_srs_list(cell_cfg, use_special_slot_only);

    // Reserve the size of the vector and set the SRS counter of each offset to 0.
    cell.srs_res_usage.reserve(cell.cell_srs_res_list.size());
    cell.srs_res_usage.assign(cell.cell_srs_res_list.size(), 0U);

    ocudu_assert(cell_cfg.ul_cfg_common.init_ul_bwp.pusch_cfg_common.has_value() and
                     cell_cfg.ue_ded_serv_cell_cfg.ul_config.has_value() and
                     cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.pucch_cfg.has_value(),
                 "The SRS aperiodic configuration generation requires PUSCH Config Common, UL Config and PUCCH Config");

    cell.slot_offsets = compute_slot_offsets(cell_cfg);
    ocudu_assert((cell_cfg.tdd_ul_dl_cfg_common.has_value() and cell.slot_offsets.size() >= 2) or
                     cell.slot_offsets.size() == 1,
                 "At least 2 SRS slot offset values expected for TDD, only 1 for FDD");
  }
}

bool du_srs_aperiodic_res_mng::alloc_resources(cell_group_config& cell_grp_cfg)
{
  for (auto& cell_cfg_ded : cell_grp_cfg.cells) {
    auto& ue_du_cell = cells[cell_cfg_ded.serv_cell_cfg.cell_index];

    // The UE SRS configuration is taken from a base configuration, saved in the GNB. The UE specific parameters will be
    // added later on in this function.
    cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.emplace(ue_du_cell.default_srs_cfg);

    srs_config& ue_srs_cfg = cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.value();

    // Find the best resource ID this UE, according to the class policy.
    const auto opt_srs_res_it = std::min_element(ue_du_cell.srs_res_usage.begin(),
                                                 ue_du_cell.srs_res_usage.end(),
                                                 [](const unsigned lhs, const unsigned rhs) { return lhs < rhs; });
    ;
    ocudu_assert(opt_srs_res_it != ue_du_cell.srs_res_usage.end(), "No SRS resource returned from a non-emtpy set");

    auto opt_res_idx = std::distance(ue_du_cell.srs_res_usage.begin(), opt_srs_res_it);

    const auto& du_res_it = ue_du_cell.get_du_srs_res_cfg(static_cast<unsigned>(opt_res_idx));
    ocudu_assert(du_res_it != ue_du_cell.cell_srs_res_list.end(), "The provided cell-ID is invalid");
    const auto& du_res = *du_res_it;

    // Update the SRS configuration with the parameters that are specific to this resource and for this UE.
    auto& only_ue_srs_res = ue_srs_cfg.srs_res_list.front();
    ue_du_cell.fill_srs_res_parameters(only_ue_srs_res, du_res);
    ue_du_cell.fill_srs_res_sets(ue_srs_cfg.srs_res_set_list, only_ue_srs_res.id.ue_res_id, ue_du_cell.slot_offsets);

    // Update the counter of UEs using this resource.
    ++ue_du_cell.srs_res_usage[opt_res_idx];
  }

  return true;
}

void du_srs_aperiodic_res_mng::cell_context::fill_srs_res_parameters(srs_config::srs_resource& res_out,
                                                                     const du_srs_resource&    res_in) const
{
  // NOTE: given that there is only 1 SRS resource per UE, we can assume that the SRS resource ID is 0.
  res_out.id.cell_res_id = res_in.cell_res_id;
  res_out.id.ue_res_id   = static_cast<srs_config::srs_res_id>(0U);
  ocudu_assert(cell_cfg.ul_carrier.nof_ant == 1 or cell_cfg.ul_carrier.nof_ant == 2 or
                   cell_context::cell_cfg.ul_carrier.nof_ant == 4,
               "The number of UL antenna ports is not valid");
  res_out.nof_ports                    = srs_config::srs_resource::nof_srs_ports::port1;
  res_out.tx_comb.size                 = cell_cfg.srs_cfg.tx_comb;
  res_out.tx_comb.tx_comb_offset       = res_in.tx_comb_offset.value();
  res_out.tx_comb.tx_comb_cyclic_shift = res_in.cs;
  res_out.freq_domain_pos              = res_in.freq_dom_position;
  res_out.res_mapping.start_pos        = NOF_OFDM_SYM_PER_SLOT_NORMAL_CP - res_in.symbols.start() - 1;
  res_out.res_mapping.nof_symb         = static_cast<srs_nof_symbols>(res_in.symbols.length());
  res_out.sequence_id                  = res_in.sequence_id;

  // Update the SRS configuration with the parameters that are common to the cell.
  res_out.freq_hop.c_srs = srs_common_params.c_srs;
  // We assume that the frequency hopping is disabled. Refer to Section 6.4.1.4.3, TS 38.211.
  res_out.freq_hop.b_srs    = 0U;
  res_out.freq_hop.b_hop    = 0U;
  res_out.freq_domain_shift = srs_common_params.freq_shift;
}

void du_srs_aperiodic_res_mng::cell_context::fill_srs_res_sets(srs_set_t&             srs_res_set_list,
                                                               srs_config::srs_res_id res_id,
                                                               span<const unsigned>   slot_offset_values) const
{
  ocudu_assert(not slot_offset_values.empty() and slot_offset_values.size() <= 3U, "Invalid number of slot_offsets");

  // Update the parameters.
  auto& srs_res_set = srs_res_set_list.front();
  srs_res_set.p0    = srs_common_params.p0;

  // The basic config has 1 SRS resource set; we expand it to (max) 3 element, by copying the first one and then
  // updating the res set ID, slot_offset and code trigger.
  for (auto it = slot_offset_values.begin(); it != slot_offset_values.end(); ++it) {
    unsigned idx = std::distance(slot_offset_values.begin(), it);

    if (it != slot_offset_values.begin()) {
      srs_res_set_list.emplace_back(srs_res_set);
    }
    auto& srs_set = srs_res_set_list.back();
    // Index the SRS resource set ID consecutively from 0 to 3.
    srs_set.id               = static_cast<srs_config::srs_res_set_id>(idx);
    auto& aperiodic_set_type = std::get<srs_config::srs_resource_set::aperiodic_resource_type>(srs_set.res_type);
    aperiodic_set_type.slot_offset.emplace(*it);
    // We use the following map to activate the SRS sets (ref to Table 7.3.1.1.2-24, TS 38.212).
    // SRS resource set 0 -> aperiodic_srs_res_trigger 1.
    // SRS resource set 1 -> aperiodic_srs_res_trigger 2.
    // (If present) SRS resource set 2 -> aperiodic_srs_res_trigger 3.
    aperiodic_set_type.aperiodic_srs_res_trigger = static_cast<uint8_t>(srs_res_set_list.back().id) + 1U;
  }
}

void du_srs_aperiodic_res_mng::dealloc_resources(cell_group_config& cell_grp_cfg)
{
  for (auto& cell_cfg_ded : cell_grp_cfg.cells) {
    // This is the cell index inside the DU.
    auto& ue_du_cell = cells[cell_cfg_ded.serv_cell_cfg.cell_index];

    if (not cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.has_value()) {
      continue;
    }

    const auto& ue_srs_cfg = cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.value();

    for (const auto& srs_res : ue_srs_cfg.srs_res_list) {
      const unsigned res_id_to_deallocate = srs_res.id.cell_res_id;

      ocudu_assert(res_id_to_deallocate < ue_du_cell.srs_res_usage.size(),
                   "The slot resource counter is expected to be non-zero");
      // Update the used_not_full slot vector.gnb
      ocudu_assert(ue_du_cell.srs_res_usage[res_id_to_deallocate] != 0,
                   "The slot resource counter is expected to be non-zero");
      --ue_du_cell.srs_res_usage[res_id_to_deallocate];
    }

    // Reset the SRS configuration in this UE. This makes sure the DU will exit this function immediately when it gets
    // called again for the same UE (upon destructor's call).
    cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.reset();
  }
}
