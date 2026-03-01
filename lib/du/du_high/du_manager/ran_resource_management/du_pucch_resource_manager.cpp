// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "du_pucch_resource_manager.h"
#include "ocudu/ran/csi_report/csi_report_config_helpers.h"
#include "ocudu/ran/csi_report/csi_report_on_pucch_helpers.h"
#include "ocudu/ran/csi_rs/csi_meas_config.h"
#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/ran/pucch/pucch_info.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/serv_cell_index.h"
#include "ocudu/scheduler/config/cell_bwp_config.h"
#include "ocudu/scheduler/config/pucch_resource_generator.h"
#include "ocudu/scheduler/config/ran_cell_config_helper.h"
#include "ocudu/scheduler/config/sched_cell_config_helpers.h"
#include "ocudu/scheduler/config/serving_cell_config.h"
#include "ocudu/scheduler/config/ue_bwp_config.h"
#include <limits>
#include <numeric>
#include <utility>

using namespace ocudu;
using namespace odu;

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

  cell.bwp_params     = cell_cfg.init_bwp_builder;
  cell.cell_pucch_cfg = make_cell_bwp_config(cell_cfg).ul.pucch;
  // TODO: remove these after we get rid of \c serving_cell_config.
  cell.default_pucch_cfg = config_helpers::make_pucch_config(cell_cfg);
  ocudu_assert(not cell.default_pucch_cfg.sr_res_list.empty(), "There must be at least one SR Resource");
  if (cell.bwp_params.csi.has_value() and not cell.bwp_params.csi->enable_aperiodic_report) {
    cell.default_csi_report_cfg = config_helpers::make_csi_meas_config(cell_cfg).value().csi_report_cfg_list[0];
  }

  // Compute SR period and SR configuration list.
  // TODO: Handle more than one SR period.
  cell.sr_period_slots = sr_periodicity_to_slot(cell.default_pucch_cfg.sr_res_list[0].period);
  for (unsigned res = 0; res < cell.bwp_params.pucch.resources.nof_cell_sr_resources; ++res) {
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
      cell.free_sr_configs.emplace_back(periodic_pucch_config{res, offset});
    }
  }

  // Compute CSI period and CSI configuration list (if periodic CSI reporting is configured).
  // TODO: Handle more than one CSI report period.
  if (cell.default_csi_report_cfg.has_value()) {
    cell.csi_period_slots = csi_resource_periodicity_to_uint(cell.bwp_params.csi->csi_rs_period);
    // Compute the LCM of SR and CSI periods, as they might not be multiples of each other.
    cell.lcm_csi_sr_period = std::lcm(cell.sr_period_slots, cell.csi_period_slots);
    for (unsigned res = 0; res < cell.bwp_params.pucch.resources.nof_cell_csi_resources; ++res) {
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
        cell.free_csi_configs.emplace_back(periodic_pucch_config{res, offset});
      }
    }
  } else {
    cell.lcm_csi_sr_period = cell.sr_period_slots;
    cell.csi_period_slots  = 0;
  }

  cell.periodic_pucchs_per_slot.resize(cell.lcm_csi_sr_period, 0);
  cells.emplace(cell_idx, std::move(cell));
}

void du_pucch_resource_manager::rem_cell(du_cell_index_t cell_idx)
{
  ocudu_assert(cells.contains(cell_idx), "Cell index={} has not been configured", cell_idx);

  cells.erase(cell_idx);
}

bool du_pucch_resource_manager::alloc_resources(cell_group_config& cell_grp_cfg)
{
  auto& cell_cfg         = cell_grp_cfg.cells.at(SERVING_PCELL_IDX);
  auto& serv_cell_cfg    = cell_cfg.serv_cell_cfg;
  auto& cell_ctx         = cells[serv_cell_cfg.cell_index];
  auto& free_sr_configs  = cell_ctx.free_sr_configs;
  auto& free_csi_configs = cell_ctx.free_csi_configs;

  // Verify where there are SR and CSI resources to allocate a new UE.
  if (free_sr_configs.empty() or (cell_ctx.default_csi_report_cfg.has_value() and free_csi_configs.empty())) {
    disable_pucch_cfg(serv_cell_cfg, cell_ctx);
    return false;
  }

  unsigned max_pucch_payload;
  if (std::holds_alternative<pucch_f2_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params)) {
    const auto& f2_params = std::get<pucch_f2_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params);
    max_pucch_payload     = get_pucch_format2_max_payload(
        f2_params.max_nof_rbs.value(), f2_params.nof_syms.value(), to_max_code_rate_float(f2_params.max_code_rate));
  } else if (std::holds_alternative<pucch_f3_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params)) {
    const auto& f3_params = std::get<pucch_f3_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params);
    max_pucch_payload     = get_pucch_format3_max_payload(f3_params.max_nof_rbs.value(),
                                                      f3_params.nof_syms.value(),
                                                      to_max_code_rate_float(f3_params.max_code_rate),
                                                      f3_params.intraslot_freq_hopping,
                                                      f3_params.additional_dmrs,
                                                      f3_params.pi2_bpsk);
  } else {
    const auto& f4_params = std::get<pucch_f4_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params);
    max_pucch_payload     = get_pucch_format4_max_payload(f4_params.nof_syms.value(),
                                                      to_max_code_rate_float(f4_params.max_code_rate),
                                                      f4_params.intraslot_freq_hopping,
                                                      f4_params.additional_dmrs,
                                                      f4_params.pi2_bpsk,
                                                      f4_params.occ_length);
  }

  std::optional<periodic_pucch_config> sr_cfg;
  std::optional<periodic_pucch_config> csi_cfg;
  for (auto sr_cfg_it = free_sr_configs.begin(); sr_cfg_it != free_sr_configs.end(); ++sr_cfg_it) {
    // Skip this SR configuration if it exceeds the limit of PUCCH grants.
    if (offset_exceeds_limit(cell_ctx, sr_cfg_it->offset, false)) {
      continue;
    }

    if (not cell_ctx.default_csi_report_cfg.has_value()) {
      // No CSI report to allocate. Allocation successful.
      sr_cfg = *sr_cfg_it;
      free_sr_configs.erase(sr_cfg_it);
      break;
    }

    serv_cell_cfg.csi_meas_cfg->csi_report_cfg_list = {*cell_ctx.default_csi_report_cfg};
    const auto csi_report_cfg                       = create_csi_report_configuration(*serv_cell_cfg.csi_meas_cfg);
    const auto csi_report_size                      = get_csi_report_pucch_size(csi_report_cfg);
    auto       optimal_res_it                       = get_compatible_csi_cfg(
        cell_ctx, *sr_cfg_it, free_csi_configs, max_pucch_payload, csi_report_size.part1_size.value());

    if (optimal_res_it != free_csi_configs.end()) {
      // Allocation successful. Remove SR and CSI resources assigned to this UE from the lists of free resources.
      sr_cfg = *sr_cfg_it;
      free_sr_configs.erase(sr_cfg_it);
      csi_cfg = *optimal_res_it;
      free_csi_configs.erase(optimal_res_it);
      break;
    }
  }

  if (not sr_cfg.has_value()) {
    // No suitable configuration found. Allocation failed.
    disable_pucch_cfg(serv_cell_cfg, cell_ctx);
    return false;
  }

  // Update the PUCCH grants-per-slot counters.
  for (auto offset :
       compute_periodic_uci_slot_offsets(cell_ctx, sr_cfg->offset, csi_cfg.has_value() ? csi_cfg->offset : 0)) {
    ocudu_assert(offset < static_cast<unsigned>(cell_ctx.periodic_pucchs_per_slot.size()),
                 "Index exceeds the size of the PUCCH grants vector");
    ++cell_ctx.periodic_pucchs_per_slot[offset];
  }

  // Update the BWP configuration for this UE with the allocated SR and CSI resources.
  auto& ul_bwp = cell_cfg.bwps[0].ul;
  ul_bwp.pucch.res_set_cfg_id =
      pucch_resource_set_config_id(cell_ctx.ue_idx % cell_ctx.bwp_params.pucch.resources.nof_cell_res_set_configs);
  ul_bwp.pucch.sr_res_id = pucch_sr_resource_id(sr_cfg->res);
  ul_bwp.pucch.sr_offset = sr_cfg->offset;
  if (csi_cfg.has_value()) {
    auto& periodic_csi_report_cfg        = ul_bwp.periodic_csi_report.emplace();
    periodic_csi_report_cfg.pucch_res_id = pucch_csi_resource_id(csi_cfg->res);
    periodic_csi_report_cfg.offset       = csi_cfg->offset;
  } else {
    ul_bwp.periodic_csi_report.reset();
  }
  update_serv_cell_cfg(serv_cell_cfg, cell_ctx, ul_bwp, max_pucch_payload);

  ++cell_ctx.ue_idx;
  return true;
}

void du_pucch_resource_manager::dealloc_resources(cell_group_config& cell_grp_cfg)
{
  auto&       serv_cell_cfg = cell_grp_cfg.cells.at(SERVING_PCELL_IDX).serv_cell_cfg;
  const auto& ul_bwp        = cell_grp_cfg.cells.at(SERVING_PCELL_IDX).bwps[0].ul;
  auto&       cell_ctx      = cells[serv_cell_cfg.cell_index];

  if (not serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg.has_value()) {
    return;
  }

  // Return SR resource/offset to the free list.
  cell_ctx.free_sr_configs.emplace_back(periodic_pucch_config{ul_bwp.pucch.sr_res_id.value(), ul_bwp.pucch.sr_offset});

  ocudu_assert(cell_ctx.default_csi_report_cfg.has_value() == ul_bwp.periodic_csi_report.has_value(),
               "Periodic CSI report configuration presence mismatch between cell and UE");
  // Return CSI resource/offset to the free list if periodic CSI is configured for this cell.
  if (cell_ctx.default_csi_report_cfg.has_value()) {
    cell_ctx.free_csi_configs.emplace_back(
        periodic_pucch_config{ul_bwp.periodic_csi_report->pucch_res_id.value(), ul_bwp.periodic_csi_report->offset});
  }

  // Remove the SR and CSI offsets from the PUCCH grants-per-slot counter.
  for (auto offset : compute_periodic_uci_slot_offsets(
           cell_ctx,
           ul_bwp.pucch.sr_offset,
           ul_bwp.periodic_csi_report.has_value() ? ul_bwp.periodic_csi_report->offset : 0)) {
    ocudu_assert(offset < cell_ctx.periodic_pucchs_per_slot.size(),
                 "Index exceeds the size of the PUCCH grants vector");
    ocudu_assert(cell_ctx.periodic_pucchs_per_slot[offset] != 0,
                 "Periodic PUCCH grants per slot counter is already at zero for offset={}",
                 offset);
    --cell_ctx.periodic_pucchs_per_slot[offset];
  }

  // Disable the PUCCH configuration in this UE. This makes sure the DU will exit this function immediately when it
  // gets called again for the same UE (upon destructor's call).
  disable_pucch_cfg(serv_cell_cfg, cell_ctx);
}

std::vector<du_pucch_resource_manager::periodic_pucch_config>::const_iterator
du_pucch_resource_manager::get_compatible_csi_cfg(const cell_resource_context&              cell_ctx,
                                                  const periodic_pucch_config&              sr_cfg,
                                                  const std::vector<periodic_pucch_config>& free_csi_list,
                                                  unsigned                                  max_pucch_payload,
                                                  unsigned                                  csi_report_size) const
{
  const pucch_resource& sr_res = cell_ctx.cell_pucch_cfg.resources.at(
      cell_ctx.bwp_params.pucch.resources.get_sr_cell_res_idx(pucch_sr_resource_id(sr_cfg.res)));
  const auto sr_symbols    = ofdm_symbol_range::start_and_len(sr_res.starting_sym_idx, sr_res.nof_symbols);
  const auto is_compatible = [&](const periodic_pucch_config& csi_cfg) {
    // Ensure the max PUCCH grants limit is not exceeded.
    if (offset_exceeds_limit(cell_ctx, csi_cfg.offset, true)) {
      return false;
    }

    // Ensure the CSI and SR resources collide in OFDM symbols, so they are multiplexed.
    const pucch_resource& csi_res = cell_ctx.cell_pucch_cfg.resources.at(
        cell_ctx.bwp_params.pucch.resources.get_csi_cell_res_idx(pucch_csi_resource_id(csi_cfg.res)));
    const auto csi_symbols = ofdm_symbol_range::start_and_len(csi_res.starting_sym_idx, csi_res.nof_symbols);
    if (not csi_symbols.overlaps(sr_symbols)) {
      return false;
    }

    // Ensure the SR and CSI reports can be sent together in the same PUCCH if their offsets collide.
    const unsigned nof_sr_bits        = sr_csi_offsets_collide(cell_ctx, sr_cfg.offset, csi_cfg.offset) ? 1 : 0;
    const unsigned candidate_uci_bits = nof_sr_bits + csi_report_size;
    return candidate_uci_bits <= max_pucch_payload;
  };

  // [Implementation-defined] CSI resource and report periods are the same.
  // TODO: Support more than one nzp-CSI-RS resource for measurement.
  const unsigned csi_rs_period   = cell_ctx.csi_period_slots;
  const unsigned csi_rs_offset   = cell_ctx.bwp_params.csi->meas_csi_slot_offset;
  const auto     weight_function = [&](const periodic_pucch_config& candidate_csi_cfg) -> unsigned {
    // [Implementation-defined] Given that it takes some time for a UE to process a CSI-RS and integrate its estimate
    // in the following CSI report, we consider a minimum slot distance before which CSI report slot offsets should be
    // avoided.
    static constexpr unsigned MINIMUM_CSI_RS_REPORT_DISTANCE = 4;

    // Prioritize offsets equal or after the \c csi_rs_slot_offset + MINIMUM_CSI_RS_REPORT_DISTANCE.
    unsigned weight =
        (csi_rs_period + candidate_csi_cfg.offset - csi_rs_offset - MINIMUM_CSI_RS_REPORT_DISTANCE) % csi_rs_period;

    // We increase the weight if the CSI report offset collides with an SR slot offset.
    if (sr_csi_offsets_collide(cell_ctx, sr_cfg.offset, candidate_csi_cfg.offset)) {
      weight += csi_rs_period;
    }

    return weight;
  };

  auto     best        = free_csi_list.end();
  unsigned best_weight = std::numeric_limits<unsigned>::max();
  for (auto csi_cfg = free_csi_list.begin(); csi_cfg != free_csi_list.end(); ++csi_cfg) {
    if (not is_compatible(*csi_cfg)) {
      continue;
    }

    const unsigned weight = weight_function(*csi_cfg);
    if (weight < best_weight) {
      best        = csi_cfg;
      best_weight = weight;
    }
  }
  return best;
}

bool du_pucch_resource_manager::offset_exceeds_limit(const cell_resource_context& cell_ctx,
                                                     unsigned                     offset,
                                                     bool                         csi) const
{
  for (unsigned off = offset, period = csi ? cell_ctx.csi_period_slots : cell_ctx.sr_period_slots;
       off < cell_ctx.lcm_csi_sr_period;
       off += period) {
    ocudu_assert(off < static_cast<unsigned>(cell_ctx.periodic_pucchs_per_slot.size()),
                 "Index exceeds the size of the PUCCH grants vector");
    if (cell_ctx.periodic_pucchs_per_slot[off] >= max_pucch_grants_per_slot) {
      return true;
    }
  }
  return false;
}

std::set<unsigned> du_pucch_resource_manager::compute_periodic_uci_slot_offsets(const cell_resource_context& cell_ctx,
                                                                                unsigned                     sr_offset,
                                                                                unsigned                     csi_offset)
{
  std::set<unsigned> slot_offsets;
  for (unsigned off = sr_offset; off < cell_ctx.lcm_csi_sr_period; off += cell_ctx.sr_period_slots) {
    slot_offsets.emplace(off);
  }

  if (cell_ctx.default_csi_report_cfg.has_value()) {
    for (unsigned off = csi_offset; off < cell_ctx.lcm_csi_sr_period; off += cell_ctx.csi_period_slots) {
      slot_offsets.emplace(off);
    }
  }
  return slot_offsets;
}

bool du_pucch_resource_manager::sr_csi_offsets_collide(const cell_resource_context& cell_ctx,
                                                       unsigned                     sr_offset,
                                                       unsigned                     csi_offset)
{
  // The CSI and SR offsets will collide if there exists a slot index s such that:
  // - s = sr_offset mod sr_period
  // - s = csi_offset mod csi_period
  // We use the Chinese Remainder Theorem to check whether a solution for s exists.
  const unsigned g = std::gcd(cell_ctx.sr_period_slots, cell_ctx.csi_period_slots);
  if (g == 1) {
    // If both periods are coprime, CRT states there will always be a solution for s for any choice of offsets.
    return true;
  }

  // Else, generalized CRT states there is a solution if and only if: i mod gcd(X, Y) = j mod gcd(X, Y), where:
  //  - i and j are the offsets for SR and CSI, respectively.
  //  - X and Y are the periods for SR and CSI, respectively.
  return (sr_offset % g) == (csi_offset % g);
}

void du_pucch_resource_manager::update_serv_cell_cfg(serving_cell_config&         serv_cell_cfg,
                                                     const cell_resource_context& cell_ctx,
                                                     const ue_uplink_bwp_config&  ul_bwp,
                                                     unsigned                     max_pucch_payload)
{
  // Fill the serving cell config with the PUCCH-related configuration.
  auto& pucch_cfg = serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg.emplace(cell_ctx.default_pucch_cfg);
  config_helpers::ue_pucch_config_builder(serv_cell_cfg,
                                          cell_ctx.cell_pucch_cfg.resources,
                                          cell_ctx.bwp_params.pucch.resources,
                                          ul_bwp.pucch,
                                          ul_bwp.periodic_csi_report);

  // TODO: remove.
  // Update the PUCCH max payload.
  // As per TS 38.231, Section 9.2.1, with PUCCH Format 1, we can have up to 2 HARQ-ACK bits (SR doesn't count as part
  // of the payload).
  static constexpr unsigned pucch_f0_f1_max_harq_payload = 2U;
  if (std::holds_alternative<pucch_f0_params>(cell_ctx.bwp_params.pucch.resources.f0_or_f1_params)) {
    pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_0)] = pucch_f0_f1_max_harq_payload;
  } else {
    pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_1)] = pucch_f0_f1_max_harq_payload;
  }
  if (std::holds_alternative<pucch_f2_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params)) {
    pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_2)] = max_pucch_payload;
    pucch_cfg.set_1_format                                                     = pucch_format::FORMAT_2;
  } else if (std::holds_alternative<pucch_f3_params>(cell_ctx.bwp_params.pucch.resources.f2_or_f3_or_f4_params)) {
    pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_3)] = max_pucch_payload;
    pucch_cfg.set_1_format                                                     = pucch_format::FORMAT_3;
  } else {
    pucch_cfg.format_max_payload[pucch_format_to_uint(pucch_format::FORMAT_4)] = max_pucch_payload;
    pucch_cfg.set_1_format                                                     = pucch_format::FORMAT_4;
  }
}

void du_pucch_resource_manager::disable_pucch_cfg(serving_cell_config&         serv_cell_cfg,
                                                  const cell_resource_context& cell_ctx)
{
  serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg.reset();
  if (cell_ctx.default_csi_report_cfg.has_value()) {
    serv_cell_cfg.csi_meas_cfg.value().csi_report_cfg_list.clear();
  }
}
