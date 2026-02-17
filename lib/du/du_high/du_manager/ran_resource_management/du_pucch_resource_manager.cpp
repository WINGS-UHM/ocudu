/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_pucch_resource_manager.h"
#include "ocudu/du/du_cell_config_helpers.h"
#include "ocudu/ran/csi_report/csi_report_config_helpers.h"
#include "ocudu/ran/csi_report/csi_report_on_pucch_helpers.h"
#include "ocudu/ran/pucch/pucch_info.h"
#include "ocudu/scheduler/config/pucch_resource_generator.h"
#include "ocudu/scheduler/config/sched_cell_config_helpers.h"
#include "ocudu/scheduler/result/pucch_format.h"
#include <numeric>
#include <utility>

using namespace ocudu;
using namespace odu;

static bool csi_offset_exceeds_grant_cnt(unsigned                     offset_candidate,
                                         unsigned                     max_pucch_grants_per_slot,
                                         unsigned                     lcm_csi_sr_period,
                                         unsigned                     csi_period_slots,
                                         const std::vector<unsigned>& pucch_grants_per_slot_cnt)
{
  for (unsigned csi_off = offset_candidate; csi_off < lcm_csi_sr_period; csi_off += csi_period_slots) {
    if (pucch_grants_per_slot_cnt[csi_off] >= max_pucch_grants_per_slot) {
      return true;
    }
  }

  return false;
}

static std::optional<csi_report_config> make_default_csi_report_config(const ran_cell_config& cell_cfg)
{
  const auto csi_meas_cfg = config_helpers::make_csi_meas_config(cell_cfg);
  if (csi_meas_cfg.has_value() and not is_pusch_configured(*csi_meas_cfg)) {
    return csi_meas_cfg->csi_report_cfg_list[0];
  }
  return std::nullopt;
}

du_pucch_resource_manager::du_pucch_resource_manager(unsigned max_pucch_grants_per_slot_) :
  // Leave 1 PUCCH grant for HARQ ACKs.
  max_pucch_grants_per_slot(max_pucch_grants_per_slot_ - 1U)
{
  ocudu_assert(max_pucch_grants_per_slot_ > 0, "At least one PUCCH grant per slot is required");
}

void du_pucch_resource_manager::add_cell(du_cell_index_t cell_idx, const ran_cell_config& cell_cfg)
{
  ocudu_assert(not cells.contains(cell_idx), "Cell index={} already configured", cell_idx);

  cell_resource_context cell;

  // Configure cell-specific PUCCH parameters.
  cell.user_defined_pucch_cfg = cell_cfg.init_bwp_builder.pucch.resources;
  cell.default_pucch_res_list = config_helpers::build_pucch_resource_list(
      cell.user_defined_pucch_cfg, cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length());
  cell.default_pucch_cfg      = config_helpers::make_pucch_config(cell_cfg);
  cell.default_csi_report_cfg = make_default_csi_report_config(cell_cfg);

  ocudu_assert(not cell.default_pucch_cfg.sr_res_list.empty(), "There must be at least one SR Resource");

  // Compute fundamental SR period.
  // TODO: Handle more than one SR period.
  cell.sr_period_slots = sr_periodicity_to_slot(cell.default_pucch_cfg.sr_res_list[0].period);

  // Compute fundamental CSI report period.
  // TODO: Handle more than one CSI report period.
  cell.csi_period_slots = 0;
  if (cell.default_csi_report_cfg.has_value()) {
    const auto& rep = std::get<csi_report_config::periodic_or_semi_persistent_report_on_pucch>(
        cell.default_csi_report_cfg->report_cfg_type);
    cell.csi_period_slots = csi_report_periodicity_to_uint(rep.report_slot_period);
  }
  // As the SR and CSI period might not be one a multiple of each other, we compute the Least Common Multiple (LCM) of
  // the two periods.
  cell.lcm_csi_sr_period = std::lcm(cell.sr_period_slots, cell.csi_period_slots);

  cell.pucch_grants_per_slot_cnt.resize(cell.lcm_csi_sr_period, 0);

  // Set up SR resources and offsets.
  for (unsigned sr_res_idx = 0; sr_res_idx < cell.user_defined_pucch_cfg.nof_cell_sr_resources; ++sr_res_idx) {
    for (unsigned offset = 0; offset != cell.sr_period_slots; ++offset) {
      if (cell_cfg.tdd_ul_dl_cfg_common.has_value()) {
        const tdd_ul_dl_config_common& tdd_cfg = *cell_cfg.tdd_ul_dl_cfg_common;
        const unsigned slot_index = offset % (NOF_SUBFRAMES_PER_FRAME * get_nof_slots_per_subframe(tdd_cfg.ref_scs));
        if (get_active_tdd_ul_symbols(tdd_cfg, slot_index, cyclic_prefix::NORMAL).length() !=
            NOF_OFDM_SYM_PER_SLOT_NORMAL_CP) {
          // UL disabled for this slot.
          continue;
        }
      }
      cell.sr_res_offset_free_list.emplace_back(sr_res_idx, offset);
    }
  }

  // Set up CSI resources and offsets.
  for (unsigned csi_res_idx = 0; csi_res_idx < cell.user_defined_pucch_cfg.nof_cell_csi_resources; ++csi_res_idx) {
    for (unsigned offset = 0; offset != cell.csi_period_slots; ++offset) {
      if (cell_cfg.tdd_ul_dl_cfg_common.has_value()) {
        const tdd_ul_dl_config_common& tdd_cfg = *cell_cfg.tdd_ul_dl_cfg_common;
        const unsigned slot_index = offset % (NOF_SUBFRAMES_PER_FRAME * get_nof_slots_per_subframe(tdd_cfg.ref_scs));
        if (get_active_tdd_ul_symbols(tdd_cfg, slot_index, cyclic_prefix::NORMAL).length() !=
            NOF_OFDM_SYM_PER_SLOT_NORMAL_CP) {
          // UL disabled for this slot.
          continue;
        }
      }
      cell.csi_res_offset_free_list.emplace_back(csi_res_idx, offset);
    }
  }

  cells.emplace(cell_idx, std::move(cell));
}

void du_pucch_resource_manager::rem_cell(du_cell_index_t cell_idx)
{
  ocudu_assert(cells.contains(cell_idx), "Cell index={} has not been configured", cell_idx);

  cells.erase(cell_idx);
}

/// This function selects a CSI report slot offset, optimizing for the following criteria:
/// - the CSI report slot offset should avoid matching the SR slot offset. This is so that we reduce the probability of
/// going above the maximum PUCCH code rate.
/// - the CSI report slot offset should be right after the CSI-RS slot offset to ensure the CSI reports are up-to-date.
std::vector<std::pair<unsigned, unsigned>>::const_iterator
du_pucch_resource_manager::find_optimal_csi_report_slot_offset(
    du_cell_index_t                                   cell_index,
    const std::vector<std::pair<unsigned, unsigned>>& available_csi_slot_offsets,
    unsigned                                          candidate_sr_offset,
    const pucch_resource&                             sr_res_cfg,
    const csi_meas_config&                            csi_meas_cfg)
{
  // [Implementation-defined] Given that it takes some time for a UE to process a CSI-RS and integrate its estimate
  // in the following CSI report, we consider a minimum slot distance before which CSI report slot offsets should be
  // avoided.
  static constexpr unsigned MINIMUM_CSI_RS_REPORT_DISTANCE = 4;

  // TODO: Support more than one nzp-CSI-RS resource for measurement.
  const csi_res_config_id_t  csi_res_cfg_id = csi_meas_cfg.csi_report_cfg_list[0].res_for_channel_meas;
  const csi_resource_config& csi_res_cfg    = csi_meas_cfg.csi_res_cfg_list[csi_res_cfg_id];
  const auto& nzp_csi_rs_ssb         = std::get<csi_resource_config::nzp_csi_rs_ssb>(csi_res_cfg.csi_rs_res_set_list);
  const auto& csi_set                = csi_meas_cfg.nzp_csi_rs_res_set_list[nzp_csi_rs_ssb.nzp_csi_rs_res_set_list[0]];
  const nzp_csi_rs_resource& csi_res = csi_meas_cfg.nzp_csi_rs_res_list[csi_set.nzp_csi_rs_res[0]];
  const unsigned             csi_rs_period = csi_resource_periodicity_to_uint(*csi_res.csi_res_period);
  const unsigned             csi_rs_offset = *csi_res.csi_res_offset;

  const auto& cell_ctx = cells[cell_index];

  const auto weight_function = [&](std::pair<unsigned, unsigned> csi_res_offset_candidate) -> unsigned {
    // This weight formula prioritizes offsets equal or after the \c csi_rs_slot_offset +
    // MINIMUM_CSI_RS_REPORT_DISTANCE.
    unsigned weight =
        (csi_rs_period + csi_res_offset_candidate.second - csi_rs_offset - MINIMUM_CSI_RS_REPORT_DISTANCE) %
        csi_rs_period;

    const pucch_resource& candidate_csi_res_cfg =
        cell_ctx.default_pucch_res_list[csi_du_res_idx_to_pucch_res_idx(cell_index, csi_res_offset_candidate.first)];
    if (sr_res_cfg.format == pucch_format::FORMAT_0 and candidate_csi_res_cfg.format == pucch_format::FORMAT_2) {
      ocudu_assert(std::holds_alternative<pucch_format_2_3_cfg>(candidate_csi_res_cfg.format_params),
                   "PUCCH resource for CSI must be of format 2");

      const ofdm_symbol_range csi_symbols = {candidate_csi_res_cfg.starting_sym_idx,
                                             candidate_csi_res_cfg.starting_sym_idx +
                                                 candidate_csi_res_cfg.nof_symbols};

      const ofdm_symbol_range sr_symbols = {sr_res_cfg.starting_sym_idx,
                                            sr_res_cfg.starting_sym_idx + sr_res_cfg.nof_symbols};

      if (not csi_symbols.overlaps(sr_symbols)) {
        weight += 2 * csi_rs_period;
        return weight;
      }
    }

    // We increase the weight if the CSI report offset collides with an SR slot offset.
    if (csi_offset_collides_with_sr(cell_index, candidate_sr_offset, csi_res_offset_candidate.second)) {
      weight += csi_rs_period;
    }

    // If the CSI offset exceeds the maximum number of PUCCH grants, we increase by 2 * csi_rs_period, to ensure this
    // gets picked as the last resort (the highest possible weight for a CSI colliding with SR
    // is = 2 * csi_rs_period - 1).
    if (csi_offset_exceeds_grant_cnt(csi_res_offset_candidate.second,
                                     max_pucch_grants_per_slot,
                                     cell_ctx.lcm_csi_sr_period,
                                     cell_ctx.csi_period_slots,
                                     cell_ctx.pucch_grants_per_slot_cnt)) {
      weight += 2 * csi_rs_period;
    }

    return weight;
  };

  auto optimal_res_it = std::min_element(
      available_csi_slot_offsets.begin(),
      available_csi_slot_offsets.end(),
      [&weight_function](const std::pair<unsigned, unsigned>& lhs, const std::pair<unsigned, unsigned>& rhs) {
        return weight_function(lhs) < weight_function(rhs);
      });

  return csi_offset_exceeds_grant_cnt(optimal_res_it->second,
                                      max_pucch_grants_per_slot,
                                      cell_ctx.lcm_csi_sr_period,
                                      cell_ctx.csi_period_slots,
                                      cell_ctx.pucch_grants_per_slot_cnt)
             ? available_csi_slot_offsets.end()
             : optimal_res_it;
}

bool du_pucch_resource_manager::alloc_resources(cell_group_config& cell_grp_cfg)
{
  auto&                 out_cell_cfg                          = cell_grp_cfg.cells[PCELL_INDEX];
  const du_cell_index_t du_cell_index                         = out_cell_cfg.serv_cell_cfg.cell_index;
  auto&                 du_cell_res_ctxt                      = cells[du_cell_index];
  auto&                 free_sr_list                          = du_cell_res_ctxt.sr_res_offset_free_list;
  auto&                 free_csi_list                         = du_cell_res_ctxt.csi_res_offset_free_list;
  out_cell_cfg.serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg = du_cell_res_ctxt.default_pucch_cfg;

  // Verify where there are SR and CSI resources to allocate a new UE.
  if (free_sr_list.empty() or (du_cell_res_ctxt.default_csi_report_cfg.has_value() and free_csi_list.empty())) {
    disable_pucch_cfg(du_cell_index, cell_grp_cfg);
    return false;
  }

  // Allocation of SR PUCCH offset.
  std::optional<std::pair<unsigned, unsigned>> sr_res_offset;
  std::optional<std::pair<unsigned, unsigned>> csi_res_offset;
  auto                                         sr_res_offset_it = free_sr_list.begin();
  // Iterate over the list of SR resource/offsets and find the first one that doesn't exceed the maximum number of PUCCH
  // grants.
  while (sr_res_offset_it != free_sr_list.end()) {
    bool pucch_cnt_exceeded = false;
    for (unsigned sr_off = sr_res_offset_it->second; sr_off < du_cell_res_ctxt.lcm_csi_sr_period;
         sr_off += du_cell_res_ctxt.sr_period_slots) {
      ocudu_assert(sr_off < static_cast<unsigned>(du_cell_res_ctxt.pucch_grants_per_slot_cnt.size()),
                   "Index exceeds the size of the PUCCH grants vector");
      pucch_cnt_exceeded = du_cell_res_ctxt.pucch_grants_per_slot_cnt[sr_off] >= max_pucch_grants_per_slot;
      if (pucch_cnt_exceeded) {
        break;
      }
    }

    // If the PUCCH count is exceeded, proceed with the next SR resource/offset pair.
    if (pucch_cnt_exceeded) {
      ++sr_res_offset_it;
      continue;
    }

    if (not du_cell_res_ctxt.default_csi_report_cfg.has_value()) {
      // No CSI report to allocate. Allocation successful.
      sr_res_offset = *sr_res_offset_it;
      free_sr_list.erase(sr_res_offset_it);
      break;
    }

    const pucch_resource sr_res =
        du_cell_res_ctxt.default_pucch_res_list[sr_du_res_idx_to_pucch_res_idx(du_cell_index, sr_res_offset_it->first)];

    out_cell_cfg.serv_cell_cfg.csi_meas_cfg->csi_report_cfg_list = {*du_cell_res_ctxt.default_csi_report_cfg};

    auto optimal_res_it = get_csi_resource_offset(du_cell_index,
                                                  out_cell_cfg.serv_cell_cfg.csi_meas_cfg.value(),
                                                  sr_res_offset_it->second,
                                                  sr_res,
                                                  free_csi_list);

    if (optimal_res_it != free_csi_list.end()) {
      // At this point the allocation has been successful. Remove SR and CSI resources assigned to this UE from the
      // lists of free resources.
      csi_res_offset = *optimal_res_it;
      free_csi_list.erase(optimal_res_it);
      sr_res_offset = *sr_res_offset_it;
      free_sr_list.erase(sr_res_offset_it);
      break;
    }

    ++sr_res_offset_it;
  }

  if (!sr_res_offset.has_value()) {
    disable_pucch_cfg(du_cell_index, cell_grp_cfg);
    return false;
  }

  // Update the PUCCH grants-per-slot counter.
  std::set<unsigned> csi_sr_offset_for_pucch_cnt = compute_sr_csi_pucch_offsets(
      du_cell_index,
      sr_res_offset.value().second,
      du_cell_res_ctxt.default_csi_report_cfg.has_value() ? csi_res_offset.value().second : 0);
  for (auto offset : csi_sr_offset_for_pucch_cnt) {
    ocudu_assert(offset < static_cast<unsigned>(du_cell_res_ctxt.pucch_grants_per_slot_cnt.size()),
                 "Index exceeds the size of the PUCCH grants vector");
    ++du_cell_res_ctxt.pucch_grants_per_slot_cnt[offset];
  }

  // Generate PUCCH resource list for the UE.
  config_helpers::ue_pucch_config_builder(out_cell_cfg.serv_cell_cfg,
                                          du_cell_res_ctxt.default_pucch_res_list,
                                          du_cell_res_ctxt.ue_idx,
                                          sr_res_offset.value().first,
                                          csi_res_offset.has_value() ? csi_res_offset.value().first : 0,
                                          du_cell_res_ctxt.user_defined_pucch_cfg.res_set_0_size,
                                          du_cell_res_ctxt.user_defined_pucch_cfg.res_set_1_size,
                                          du_cell_res_ctxt.user_defined_pucch_cfg.nof_cell_res_set_configs,
                                          du_cell_res_ctxt.user_defined_pucch_cfg.nof_cell_sr_resources,
                                          du_cell_res_ctxt.user_defined_pucch_cfg.nof_cell_csi_resources);

  // Set the offsets for SR and CSI.
  out_cell_cfg.serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg->sr_res_list.front().offset =
      sr_res_offset.value().second;
  if (out_cell_cfg.serv_cell_cfg.csi_meas_cfg.has_value() and
      not is_pusch_configured(*out_cell_cfg.serv_cell_cfg.csi_meas_cfg)) {
    std::get<csi_report_config::periodic_or_semi_persistent_report_on_pucch>(
        out_cell_cfg.serv_cell_cfg.csi_meas_cfg->csi_report_cfg_list[0].report_cfg_type)
        .report_slot_offset = csi_res_offset.value().second;
  }

  auto& ue_ded_pucch_cfg = out_cell_cfg.serv_cell_cfg.ul_config.value().init_ul_bwp.pucch_cfg.value();

  // Update the PUCCH max payload.
  // As per TS 38.231, Section 9.2.1, with PUCCH Format 1, we can have up to 2 HARQ-ACK bits (SR doesn't count as part
  // of the payload).
  static constexpr unsigned pucch_f0_f1_max_harq_payload = 2U;
  if (std::holds_alternative<pucch_f0_params>(du_cell_res_ctxt.user_defined_pucch_cfg.f0_or_f1_params)) {
    ue_ded_pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_0)] = pucch_f0_f1_max_harq_payload;
  } else {
    ue_ded_pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_1)] = pucch_f0_f1_max_harq_payload;
  }

  if (std::holds_alternative<pucch_f2_params>(du_cell_res_ctxt.user_defined_pucch_cfg.f2_or_f3_or_f4_params)) {
    auto* res_f2_it = std::find_if(ue_ded_pucch_cfg.pucch_res_list.begin(),
                                   ue_ded_pucch_cfg.pucch_res_list.end(),
                                   [](const pucch_resource& res) { return res.format == pucch_format::FORMAT_2; });
    ocudu_assert(res_f2_it != ue_ded_pucch_cfg.pucch_res_list.end(),
                 "No PUCCH F2 resource found in the UE dedicated configuration");
    ue_ded_pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_2)] = get_pucch_format2_max_payload(
        std::get<pucch_format_2_3_cfg>(res_f2_it->format_params).nof_prbs,
        res_f2_it->nof_symbols,
        to_max_code_rate_float(ue_ded_pucch_cfg.format_2_common_param.value().max_c_rate));
    ue_ded_pucch_cfg.set_1_format = pucch_format::FORMAT_2;
  } else if (std::holds_alternative<pucch_f3_params>(du_cell_res_ctxt.user_defined_pucch_cfg.f2_or_f3_or_f4_params)) {
    auto* res_f3_it = std::find_if(ue_ded_pucch_cfg.pucch_res_list.begin(),
                                   ue_ded_pucch_cfg.pucch_res_list.end(),
                                   [](const pucch_resource& res) { return res.format == pucch_format::FORMAT_3; });
    ocudu_assert(res_f3_it != ue_ded_pucch_cfg.pucch_res_list.end(),
                 "No PUCCH F3 resource found in the UE dedicated configuration");
    const auto& f3_common_params = ue_ded_pucch_cfg.format_3_common_param.value();
    ue_ded_pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_3)] =
        get_pucch_format3_max_payload(std::get<pucch_format_2_3_cfg>(res_f3_it->format_params).nof_prbs,
                                      res_f3_it->nof_symbols,
                                      to_max_code_rate_float(ue_ded_pucch_cfg.format_3_common_param.value().max_c_rate),
                                      res_f3_it->second_hop_prb.has_value(),
                                      f3_common_params.additional_dmrs,
                                      f3_common_params.pi_2_bpsk);
    ue_ded_pucch_cfg.set_1_format = pucch_format::FORMAT_3;
  } else {
    auto* res_f4_it = std::find_if(ue_ded_pucch_cfg.pucch_res_list.begin(),
                                   ue_ded_pucch_cfg.pucch_res_list.end(),
                                   [](const pucch_resource& res) { return res.format == pucch_format::FORMAT_4; });
    ocudu_assert(res_f4_it != ue_ded_pucch_cfg.pucch_res_list.end(),
                 "No PUCCH F4 resource found in the UE dedicated configuration");
    const auto& f4_common_params = ue_ded_pucch_cfg.format_4_common_param.value();
    ue_ded_pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_4)] =
        get_pucch_format4_max_payload(res_f4_it->nof_symbols,
                                      to_max_code_rate_float(ue_ded_pucch_cfg.format_4_common_param.value().max_c_rate),
                                      res_f4_it->second_hop_prb.has_value(),
                                      f4_common_params.additional_dmrs,
                                      f4_common_params.pi_2_bpsk,
                                      std::get<pucch_format_4_cfg>(res_f4_it->format_params).occ_length);
    ue_ded_pucch_cfg.set_1_format = pucch_format::FORMAT_4;
  }

  ++du_cell_res_ctxt.ue_idx;
  return true;
}

void du_pucch_resource_manager::dealloc_resources(cell_group_config& cell_grp_cfg)
{
  auto&                 out_cell_cfg  = cell_grp_cfg.cells[PCELL_INDEX].serv_cell_cfg;
  const du_cell_index_t du_cell_index = out_cell_cfg.cell_index;
  auto&                 du_cell_res   = cells[du_cell_index];

  if (not out_cell_cfg.ul_config->init_ul_bwp.pucch_cfg.has_value()) {
    return;
  }
  auto& sr_to_deallocate = out_cell_cfg.ul_config->init_ul_bwp.pucch_cfg->sr_res_list.front();
  du_cell_res.sr_res_offset_free_list.emplace_back(
      pucch_res_idx_to_sr_du_res_idx(du_cell_index, sr_to_deallocate.pucch_res_id.cell_res_id),
      sr_to_deallocate.offset);

  unsigned csi_offset = 0;
  if (out_cell_cfg.csi_meas_cfg.has_value() and not is_pusch_configured(*out_cell_cfg.csi_meas_cfg)) {
    auto& target_csi_cfg = std::get<csi_report_config::periodic_or_semi_persistent_report_on_pucch>(
        out_cell_cfg.csi_meas_cfg->csi_report_cfg_list[0].report_cfg_type);
    csi_offset = target_csi_cfg.report_slot_offset;
    du_cell_res.csi_res_offset_free_list.emplace_back(
        pucch_res_idx_to_csi_du_res_idx(du_cell_index,
                                        target_csi_cfg.pucch_csi_res_list.front().pucch_res_id.cell_res_id),
        csi_offset);
  }

  // Remove the SR and CSI offsets from the PUCCH grants-per-slot counter.
  std::set<unsigned> csi_sr_offset_for_pucch_cnt =
      compute_sr_csi_pucch_offsets(du_cell_index, sr_to_deallocate.offset, csi_offset);
  for (auto offset : csi_sr_offset_for_pucch_cnt) {
    ocudu_assert(offset < du_cell_res.pucch_grants_per_slot_cnt.size(),
                 "Index exceeds the size of the PUCCH grants vector");
    ocudu_assert(du_cell_res.pucch_grants_per_slot_cnt[offset] != 0,
                 "Index exceeds the size of the PUCCH grants vector");
    --du_cell_res.pucch_grants_per_slot_cnt[offset];
  }

  // Disable the PUCCH configuration in this UE. This makes sure the DU will exit this function immediately when it gets
  // called again for the same UE (upon destructor's call).
  disable_pucch_cfg(du_cell_index, cell_grp_cfg);
}

std::vector<std::pair<unsigned, unsigned>>::const_iterator
du_pucch_resource_manager::get_csi_resource_offset(du_cell_index_t        cell_index,
                                                   const csi_meas_config& csi_meas_cfg,
                                                   unsigned               candidate_sr_offset,
                                                   const pucch_resource&  sr_res_cfg,
                                                   const std::vector<std::pair<unsigned, unsigned>>& free_csi_list)
{
  // Chosse the optimal CSI-RS slot offset.
  auto optimal_res_it =
      find_optimal_csi_report_slot_offset(cell_index, free_csi_list, candidate_sr_offset, sr_res_cfg, csi_meas_cfg);

  if (optimal_res_it != free_csi_list.end()) {
    const auto& cell_ctx = cells[cell_index];

    // Set temporarily CSI report with a default PUCCH_res_id.
    const unsigned lowest_period       = std::min(cell_ctx.sr_period_slots, cell_ctx.csi_period_slots);
    const bool sr_csi_on_the_same_slot = candidate_sr_offset % lowest_period == optimal_res_it->second % lowest_period;

    const auto csi_report_cfg  = create_csi_report_configuration(csi_meas_cfg);
    const auto csi_report_size = get_csi_report_pucch_size(csi_report_cfg);

    const sr_nof_bits sr_bits_to_report  = sr_csi_on_the_same_slot ? sr_nof_bits::one : sr_nof_bits::no_sr;
    const unsigned    candidate_uci_bits = sr_nof_bits_to_uint(sr_bits_to_report) + csi_report_size.part1_size.value();

    unsigned max_payload;
    if (std::holds_alternative<pucch_f2_params>(cell_ctx.user_defined_pucch_cfg.f2_or_f3_or_f4_params)) {
      const auto& f2_params = std::get<pucch_f2_params>(cell_ctx.user_defined_pucch_cfg.f2_or_f3_or_f4_params);
      const float max_pucch_code_rate =
          to_max_code_rate_float(cell_ctx.default_pucch_cfg.format_2_common_param.value().max_c_rate);
      max_payload =
          get_pucch_format2_max_payload(f2_params.max_nof_rbs, f2_params.nof_symbols.value(), max_pucch_code_rate);
    } else if (std::holds_alternative<pucch_f3_params>(cell_ctx.user_defined_pucch_cfg.f2_or_f3_or_f4_params)) {
      const auto& f3_params = std::get<pucch_f3_params>(cell_ctx.user_defined_pucch_cfg.f2_or_f3_or_f4_params);
      const float max_pucch_code_rate =
          to_max_code_rate_float(cell_ctx.default_pucch_cfg.format_3_common_param.value().max_c_rate);
      max_payload = get_pucch_format3_max_payload(f3_params.max_nof_rbs,
                                                  f3_params.nof_symbols.value(),
                                                  max_pucch_code_rate,
                                                  f3_params.intraslot_freq_hopping,
                                                  f3_params.additional_dmrs,
                                                  f3_params.pi2_bpsk);
    } else {
      const auto& f4_params = std::get<pucch_f4_params>(cell_ctx.user_defined_pucch_cfg.f2_or_f3_or_f4_params);
      const float max_pucch_code_rate =
          to_max_code_rate_float(cell_ctx.default_pucch_cfg.format_4_common_param.value().max_c_rate);
      max_payload = get_pucch_format4_max_payload(f4_params.nof_symbols.value(),
                                                  max_pucch_code_rate,
                                                  f4_params.intraslot_freq_hopping,
                                                  f4_params.additional_dmrs,
                                                  f4_params.pi2_bpsk,
                                                  f4_params.occ_length);
    }

    // This is the case of suitable CSI offset found, but the CSI offset would result in exceeding the PUCCH F2/F3/F4
    // max payload.
    if (candidate_uci_bits > max_payload) {
      // Allocation failed, exit without allocating the UE resources.
      optimal_res_it = free_csi_list.end();
    }
  }

  return optimal_res_it;
}

std::set<unsigned> du_pucch_resource_manager::compute_sr_csi_pucch_offsets(du_cell_index_t cell_index,
                                                                           unsigned        sr_offset,
                                                                           unsigned        csi_offset)
{
  const auto&        cell_ctx = cells[cell_index];
  std::set<unsigned> sr_csi_offsets;
  for (unsigned sr_off = sr_offset; sr_off < cell_ctx.lcm_csi_sr_period; sr_off += cell_ctx.sr_period_slots) {
    sr_csi_offsets.emplace(sr_off);
  }

  if (cell_ctx.default_csi_report_cfg.has_value()) {
    for (unsigned csi_off = csi_offset; csi_off < cell_ctx.lcm_csi_sr_period; csi_off += cell_ctx.csi_period_slots) {
      sr_csi_offsets.emplace(csi_off);
    }
  }
  return sr_csi_offsets;
}

bool du_pucch_resource_manager::csi_offset_collides_with_sr(du_cell_index_t cell_index,
                                                            unsigned        sr_offset,
                                                            unsigned        csi_offset) const
{
  const auto& cell_ctx = cells[cell_index];
  for (unsigned csi_off = csi_offset; csi_off < cell_ctx.lcm_csi_sr_period; csi_off += cell_ctx.csi_period_slots) {
    for (unsigned sr_off = sr_offset; sr_off < cell_ctx.lcm_csi_sr_period; sr_off += cell_ctx.sr_period_slots) {
      if (csi_off == sr_off) {
        return true;
      }
    }
  }

  return false;
}

unsigned du_pucch_resource_manager::sr_du_res_idx_to_pucch_res_idx(du_cell_index_t cell_index,
                                                                   unsigned        sr_du_res_idx) const
{
  const auto& pucch_cfg = cells[cell_index].user_defined_pucch_cfg;
  // The mapping from the UE's PUCCH-Config \ref res_id index to the DU index for PUCCH SR resource is the inverse of
  // what is defined in \ref odu::ue_pucch_config_builder.
  return pucch_cfg.res_set_0_size.value() * pucch_cfg.nof_cell_res_set_configs + sr_du_res_idx;
}

unsigned du_pucch_resource_manager::pucch_res_idx_to_sr_du_res_idx(du_cell_index_t cell_index,
                                                                   unsigned        pucch_res_idx) const
{
  const auto& pucch_cfg = cells[cell_index].user_defined_pucch_cfg;
  // The mapping from the UE's PUCCH-Config \ref res_id index to the DU index for PUCCH SR resource is the inverse of
  // what is defined in \ref odu::ue_pucch_config_builder.
  return pucch_res_idx - pucch_cfg.res_set_0_size.value() * pucch_cfg.nof_cell_res_set_configs;
}

unsigned du_pucch_resource_manager::csi_du_res_idx_to_pucch_res_idx(du_cell_index_t cell_index,
                                                                    unsigned        csi_du_res_idx) const
{
  const auto& pucch_cfg = cells[cell_index].user_defined_pucch_cfg;
  // The mapping from the UE's PUCCH-Config \ref res_id index to the DU index for PUCCH CSI resource is the inverse of
  // what is defined in \ref odu::ue_pucch_config_builder.
  return pucch_cfg.res_set_0_size.value() * pucch_cfg.nof_cell_res_set_configs + pucch_cfg.nof_cell_sr_resources +
         pucch_cfg.res_set_1_size.value() * pucch_cfg.nof_cell_res_set_configs + csi_du_res_idx;
}

unsigned du_pucch_resource_manager::pucch_res_idx_to_csi_du_res_idx(du_cell_index_t cell_index,
                                                                    unsigned        pucch_res_idx) const
{
  const auto& pucch_cfg = cells[cell_index].user_defined_pucch_cfg;
  // The mapping from the UE's PUCCH-Config \ref res_id index to the DU index for PUCCH CSI resource is the inverse of
  // what is defined in \ref odu::ue_pucch_config_builder.
  return pucch_res_idx -
         (pucch_cfg.res_set_0_size.value() * pucch_cfg.nof_cell_res_set_configs + pucch_cfg.nof_cell_sr_resources +
          pucch_cfg.res_set_1_size.value() * pucch_cfg.nof_cell_res_set_configs);
}

void du_pucch_resource_manager::disable_pucch_cfg(du_cell_index_t cell_index, cell_group_config& cell_grp_cfg)
{
  auto& serv_cell = cell_grp_cfg.cells[PCELL_INDEX].serv_cell_cfg;

  serv_cell.ul_config->init_ul_bwp.pucch_cfg.reset();
  if (cells[cell_index].default_csi_report_cfg.has_value()) {
    serv_cell.csi_meas_cfg.value().csi_report_cfg_list.clear();
  }
}
