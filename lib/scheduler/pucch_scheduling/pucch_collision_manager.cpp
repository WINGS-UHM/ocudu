/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pucch_collision_manager.h"
#include "../support/pucch/pucch_default_resource.h"
#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/adt/expected.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/pucch/pucch_mapping.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include <algorithm>

using namespace ocudu;

/// Constructs resource_info for a common PUCCH resource.
static detail::resource_info
make_common_resource_info(const pucch_default_resource& res, unsigned r_pucch, const bwp_configuration& bwp_cfg)
{
  // Compute PRB_first_hop and PRB_second_hop as per Section 9.2.1, TS 38.213.
  auto prbs = get_pucch_default_prb_index(r_pucch, res.rb_bwp_offset, res.cs_indexes.size(), bwp_cfg.crbs.length());

  return detail::resource_info{
      .format             = res.format,
      .multiplexing_index = res.cs_indexes[get_pucch_default_cyclic_shift(r_pucch, res.cs_indexes.size())],
      .grants             = {
                      .first_hop = grant_info(
              bwp_cfg.scs,
              {res.first_symbol_index, res.first_symbol_index + res.nof_symbols / 2},
              prb_to_crb(bwp_cfg, prb_interval::start_and_len(prbs.first, pucch_constants::FORMAT0_1_4_MAX_NPRB))),
                      .second_hop = grant_info(
              bwp_cfg.scs,
              {res.first_symbol_index + res.nof_symbols / 2, res.first_symbol_index + res.nof_symbols},
              prb_to_crb(bwp_cfg, prb_interval::start_and_len(prbs.second, pucch_constants::FORMAT0_1_4_MAX_NPRB))),
      }};
}

/// Constructs resource_info for a dedicated PUCCH resource.
static detail::resource_info make_ded_resource_info(const pucch_resource& res, const bwp_configuration& bwp_cfg)
{
  detail::resource_info info{.format = res.format};

  // Compute multiplexing index.
  switch (res.format) {
    case pucch_format::FORMAT_0: {
      const auto& f0_params   = std::get<pucch_format_0_cfg>(res.format_params);
      info.multiplexing_index = f0_params.initial_cyclic_shift;
    } break;
    case pucch_format::FORMAT_1: {
      // For PUCCH Format 1, two sequences are orthogonal unless both the initial cyclic shift and the time domain OCC
      // index are the same.
      const auto& f1_params = std::get<pucch_format_1_cfg>(res.format_params);
      info.multiplexing_index =
          f1_params.initial_cyclic_shift +
          f1_params.time_domain_occ * pucch_constants::format1_initial_cyclic_shift_range.length();
    } break;
    case pucch_format::FORMAT_4: {
      // For PUCCH Format 4, the OCC index is mapped to a cyclic shift value, as per Table 6.4.1.3.3.1-1, TS 38.211.
      // Thus, resources with different OCC indices will never collide, even if they have different OCC lengths.
      // Therefore, we can use the OCC index directly as the multiplexing index.
      const auto& f4_params   = std::get<pucch_format_4_cfg>(res.format_params);
      info.multiplexing_index = static_cast<unsigned>(f4_params.occ_index);
    } break;
    default:
      // Non multiplexed formats.
      info.multiplexing_index = 0;
      break;
  }

  // Compute time-frequency grants.
  unsigned nof_prbs = 1;
  if (const auto* format_2_3 = std::get_if<pucch_format_2_3_cfg>(&res.format_params)) {
    nof_prbs = format_2_3->nof_prbs;
  }
  if (res.second_hop_prb.has_value()) {
    // Intra-slot frequency hopping.
    info.grants = {
        .first_hop = grant_info{bwp_cfg.scs,
                                {res.starting_sym_idx, res.starting_sym_idx + res.nof_symbols / 2},
                                prb_to_crb(bwp_cfg, prb_interval::start_and_len(res.starting_prb, nof_prbs))},
        .second_hop =
            grant_info{bwp_cfg.scs,
                       {res.starting_sym_idx + res.nof_symbols / 2, res.starting_sym_idx + res.nof_symbols},
                       prb_to_crb(bwp_cfg, prb_interval::start_and_len(res.second_hop_prb.value(), nof_prbs))}};
  } else {
    // No intra-slot frequency hopping.
    info.grants = {
        .first_hop  = grant_info{bwp_cfg.scs,
                                ofdm_symbol_range::start_and_len(res.starting_sym_idx, res.nof_symbols),
                                prb_to_crb(bwp_cfg, prb_interval::start_and_len(res.starting_prb, nof_prbs))},
        .second_hop = std::nullopt,
    };
  }

  return info;
}

/// Checks if two PUCCH resources collide.
static bool do_resources_collide(const detail::resource_info& res1, const detail::resource_info& res2)
{
  if (not res1.grants.overlaps(res2.grants)) {
    // Resources that do not overlap in time and frequency do not collide.
    return false;
  }

  if (res1.format != res2.format) {
    // Resources with different formats always collide if they overlap in time and frequency.
    return true;
  }

  if (res1.grants != res2.grants) {
    // We can only make sure resources have orthogonal sequences if they have the same time/frequency allocation.
    return true;
  }

  // Resources with the same format and time/frequency grants only collide if they have the same multiplexing index.
  // Note: resources with Format 2/3 always collide as they are not multiplexed (multiplexing index is always 0).
  return res1.multiplexing_index == res2.multiplexing_index;
}

namespace {

/// \brief Represents a multiplexing region of PUCCH resources.
/// Contains parameters that all resources in the region have in common.
struct mux_region {
  /// Time-frequency grants of the region.
  detail::pucch_grants grants;
  /// PUCCH format of the region.
  pucch_format format;

  /// Check if a a given resource belongs to this multiplexing region.
  bool does_resource_belong(const detail::resource_info& res) const
  {
    return res.format == format and res.grants == grants;
  }
};

} // namespace

pucch_collision_manager::pucch_collision_manager(const cell_configuration& cell_cfg_) :
  cell_cfg(cell_cfg_),
  resources(compute_resources(cell_cfg)),
  collision_matrix(compute_collisions(resources)),
  mux_matrix(compute_mux_regions(resources)),
  slots_ctx({slot_context(cell_cfg)})
{
}

void pucch_collision_manager::slot_indication(slot_point sl_tx)
{
  // If last_sl_ind is not valid (not initialized), then the check sl_tx == last_sl_ind + 1 does not matter.
  ocudu_sanity_check(not last_sl_ind.valid() or sl_tx == last_sl_ind + 1, "Detected a skipped slot");

  // Clear previous slot context.
  slots_ctx[(sl_tx - 1).to_uint()].current_state.reset();
  // Update last slot indication.
  last_sl_ind = sl_tx;
}

void pucch_collision_manager::stop()
{
  // Clear all slot contexts.
  for (auto& ctx : slots_ctx) {
    ctx.current_state.reset();
  }
  last_sl_ind = {};
}

pucch_collision_manager::res_idx_t pucch_collision_manager::get_common_idx(unsigned r_pucch)
{
  ocudu_assert(r_pucch < nof_common_res,
               "Common PUCCH resource index {} exceeds the maximum allowed {}.",
               r_pucch,
               nof_common_res);
  return res_idx_t(r_pucch);
}

/// Get the internal index of the dedicated PUCCH resource inside the collision manager.
pucch_collision_manager::res_idx_t pucch_collision_manager::get_ded_idx(unsigned cell_res_id) const
{
  ocudu_assert(cell_res_id < cell_cfg.ded_pucch_resources.size(),
               "Dedicated PUCCH resource index {} exceeds the maximum allowed {}.",
               cell_res_id,
               cell_cfg.ded_pucch_resources.size());
  return res_idx_t(nof_common_res + cell_res_id);
}

const bounded_bitset<pucch_collision_manager::max_nof_cell_resources>&
pucch_collision_manager::get_collision_row(res_idx_t res_idx) const
{
  return collision_matrix[res_idx.value()];
}

const bounded_bitset<pucch_collision_manager::max_nof_cell_resources>*
pucch_collision_manager::get_mux_row(res_idx_t res_idx) const
{
  for (const auto& region : mux_matrix) {
    if (region.test(res_idx.value())) {
      return &region;
    }
  }

  return nullptr;
}

pucch_collision_manager::alloc_result_t
pucch_collision_manager::alloc(cell_slot_resource_grid& ul_res_grid, slot_point sl, res_idx_t res_idx)
{
  ocudu_sanity_check(sl < last_sl_ind + cell_resource_allocator::RING_ALLOCATOR_SIZE,
                     "PUCCH resource ring-buffer accessed too far into the future");

  auto& ctx = slots_ctx[sl.to_uint()];

  // Check for PUCCH-to-other UL grant collisions using the resource grids.
  const auto& res = resources[res_idx.value()];
  if (ul_res_grid.collides(res.grants.first_hop, &ctx.pucch_res_grid)) {
    return make_unexpected(alloc_failure_reason::UL_GRANT_COLLISION);
  }
  if (res.grants.second_hop.has_value() and ul_res_grid.collides(*res.grants.second_hop, &ctx.pucch_res_grid)) {
    return make_unexpected(alloc_failure_reason::UL_GRANT_COLLISION);
  }

  // Check for PUCCH-to-PUCCH collisions using the collision matrix.
  const auto& row = collision_matrix[res_idx.value()];
  if ((row & ctx.current_state).any()) {
    return make_unexpected(alloc_failure_reason::PUCCH_COLLISION);
  }

  // Allocate the resource.
  ctx.current_state.set(res_idx.value());

  // Fill grants in ul_res_grid and ctx.pucch_res_grid.
  ul_res_grid.fill(res.grants.first_hop);
  ctx.pucch_res_grid.fill(res.grants.first_hop);
  if (res.grants.second_hop.has_value()) {
    ul_res_grid.fill(*res.grants.second_hop);
    ctx.pucch_res_grid.fill(*res.grants.second_hop);
  }
  return default_success_t();
}

pucch_collision_manager::alloc_result_t
pucch_collision_manager::alloc_common(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned r_pucch)
{
  return alloc(ul_res_grid, sl, get_common_idx(r_pucch));
}

pucch_collision_manager::alloc_result_t
pucch_collision_manager::alloc_ded(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned cell_res_id)
{
  return alloc(ul_res_grid, sl, get_ded_idx(cell_res_id));
}

bool pucch_collision_manager::free(cell_slot_resource_grid& ul_res_grid, slot_point sl, res_idx_t res_idx)
{
  ocudu_sanity_check(sl < last_sl_ind + cell_resource_allocator::RING_ALLOCATOR_SIZE,
                     "PUCCH resource ring-buffer accessed too far into the future");

  auto& ctx = slots_ctx[sl.to_uint()];
  if (not ctx.current_state.test(res_idx.value())) {
    // Resource was not allocated.
    return false;
  }
  ctx.current_state.reset(res_idx.value());

  // Check if any other resource in the same multiplexing region is still allocated.
  // If not, clear the grants in ul_res_grid and ctx.pucch_res_grid.
  const auto* mux_row = get_mux_row(res_idx);
  if (mux_row != nullptr and (*mux_row & ctx.current_state).any()) {
    return true;
  }

  // Clear grants in ul_res_grid and ctx.pucch_res_grid.
  const auto& res = resources[res_idx.value()];
  ul_res_grid.clear(res.grants.first_hop);
  ctx.pucch_res_grid.clear(res.grants.first_hop);
  if (res.grants.second_hop.has_value()) {
    ul_res_grid.clear(*res.grants.second_hop);
    ctx.pucch_res_grid.clear(*res.grants.second_hop);
  }
  return true;
}

bool pucch_collision_manager::free_common(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned r_pucch)
{
  return free(ul_res_grid, sl, get_common_idx(r_pucch));
}

bool pucch_collision_manager::free_ded(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned cell_res_id)
{
  return free(ul_res_grid, sl, get_ded_idx(cell_res_id));
}

pucch_collision_manager::cell_resources_t pucch_collision_manager::compute_resources(const cell_configuration& cell_cfg)
{
  const auto& init_ul_bwp_cfg = cell_cfg.ul_cfg_common.init_ul_bwp.generic_params;

  // Get PUCCH common resource config from Table 9.2.1-1, TS 38.213.
  // N_bwp_size is equal to the Initial UL BWP size in PRBs, as per TS 38.213, Section 9.2.1.
  const pucch_default_resource common_default_res = get_pucch_default_resource(
      cell_cfg.ul_cfg_common.init_ul_bwp.pucch_cfg_common->pucch_resource_common, init_ul_bwp_cfg.crbs.length());

  // Collect all resources (common + dedicated).
  cell_resources_t all_resources;
  for (unsigned r_pucch = 0; r_pucch != nof_common_res; ++r_pucch) {
    all_resources.push_back(make_common_resource_info(common_default_res, r_pucch, init_ul_bwp_cfg));
  }
  for (const auto& res : cell_cfg.ded_pucch_resources) {
    all_resources.push_back(make_ded_resource_info(res, init_ul_bwp_cfg));
  }

  return all_resources;
}

pucch_collision_manager::collision_matrix_t
pucch_collision_manager::compute_collisions(span<const detail::resource_info> resources)
{
  collision_matrix_t matrix(resources.size(), bounded_bitset<max_nof_cell_resources>(resources.size()));

  // Precompute the collision matrix.
  for (size_t i = 0; i != resources.size(); ++i) {
    // A resource always collides with itself.
    matrix[i].set(i);

    // Note: The collision matrix is symmetric.
    for (size_t j = i + 1; j != resources.size(); ++j) {
      if (do_resources_collide(resources[i], resources[j])) {
        matrix[i].set(j);
        matrix[j].set(i);
      }
    }
  }

  return matrix;
}

pucch_collision_manager::mux_regions_matrix_t
pucch_collision_manager::compute_mux_regions(span<const detail::resource_info> resources)
{
  // Helper structure to keep track of multiplexing regions and their members.
  struct region_record {
    mux_region                             region;
    bounded_bitset<max_nof_cell_resources> members;
  };
  static_vector<region_record, max_nof_cell_resources> tmp_regions;

  for (size_t i = 0; i != resources.size(); ++i) {
    const auto& res = resources[i];

    // Find if the resource belongs to an existing multiplexing region.
    auto* region_it = std::find_if(tmp_regions.begin(), tmp_regions.end(), [&res](const region_record& record) {
      return record.region.does_resource_belong(res);
    });

    if (region_it == tmp_regions.end()) {
      // If the multiplexing region does not exist yet, create it.
      region_it = &tmp_regions.emplace_back(region_record{mux_region{
                                                              .grants = res.grants,
                                                              .format = res.format,
                                                          },
                                                          bounded_bitset<max_nof_cell_resources>(resources.size())});
    }

    // Add the resource to the multiplexing region.
    region_it->members.set(i);
  }

  mux_regions_matrix_t mux_regions;
  for (const auto& record : tmp_regions) {
    // Return only multiplexing regions with more than one resource.
    if (record.members.count() < 2) {
      continue;
    }

    mux_regions.push_back(record.members);
  }
  return mux_regions;
}
