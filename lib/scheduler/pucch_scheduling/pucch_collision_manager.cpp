// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "pucch_collision_manager.h"
#include "../support/pucch/pucch_default_resource.h"
#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/adt/expected.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/pucch/pucch_mapping.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/scheduler/resource_grid_util.h"
#include <algorithm>

using namespace ocudu;
using namespace ocudu::detail;

namespace ocudu::detail {

cell_resource_list make_cell_resource_list(const cell_configuration& cell_cfg)
{
  const auto& init_ul_bwp_cfg = cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params;

  // Get PUCCH common resource config from Table 9.2.1-1, TS 38.213.
  // N_bwp_size is equal to the Initial UL BWP size in PRBs, as per TS 38.213, Section 9.2.1.
  const pucch_default_resource common_default_res = get_pucch_default_resource(
      cell_cfg.params.ul_cfg_common.init_ul_bwp.pucch_cfg_common->pucch_resource_common, init_ul_bwp_cfg.crbs.length());

  // Collect all resources (common + dedicated).
  cell_resource_list all_resources;
  for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
    all_resources.push_back(pucch_collision_info(common_default_res, r_pucch, init_ul_bwp_cfg));
  }
  for (const auto& res : cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources) {
    all_resources.push_back(pucch_collision_info(res, init_ul_bwp_cfg));
  }

  return all_resources;
}

collision_matrix make_collision_matrix(const cell_resource_list& resources)
{
  collision_matrix matrix(resources.size(),
                          bounded_bitset<pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>(resources.size()));

  // Precompute the collision matrix.
  for (size_t i = 0; i != resources.size(); ++i) {
    // A resource always collides with itself.
    matrix[i].set(i);

    // Note: The collision matrix is symmetric.
    for (size_t j = i + 1; j != resources.size(); ++j) {
      if (resources[i].collides(resources[j])) {
        matrix[i].set(j);
        matrix[j].set(i);
      }
    }
  }

  return matrix;
}

mux_regions_matrix make_mux_regions_matrix(const cell_resource_list& resources)
{
  // Helper structure to keep track of multiplexing regions and their members.
  struct mux_region {
    // Time-frequency grants of the region.
    pucch_grants grants;
    // PUCCH format of the region.
    pucch_format format;
    // Members of the region.
    bounded_bitset<pucch_constants::MAX_NOF_TOT_CELL_RESOURCES> members;
    // Check if a given resource belongs to this multiplexing region.
    bool does_resource_belong(const pucch_collision_info& res) const
    {
      return res.format == format and res.grants == grants;
    }
  };
  static_vector<mux_region, pucch_constants::MAX_NOF_TOT_CELL_RESOURCES> tmp_regions;

  for (size_t i = 0; i != resources.size(); ++i) {
    const auto& res = resources[i];

    // Find if the resource belongs to an existing multiplexing region.
    auto* region_it = std::find_if(tmp_regions.begin(), tmp_regions.end(), [&res](const mux_region& region) {
      return region.does_resource_belong(res);
    });

    if (region_it == tmp_regions.end()) {
      // If the multiplexing region does not exist yet, create it.
      region_it = &tmp_regions.emplace_back(mux_region{
          res.grants, res.format, bounded_bitset<pucch_constants::MAX_NOF_TOT_CELL_RESOURCES>(resources.size())});
    }

    // Add the resource to the multiplexing region.
    region_it->members.set(i);
  }

  mux_regions_matrix mux_regions;
  for (const auto& record : tmp_regions) {
    // Return only multiplexing regions with more than one resource.
    if (record.members.count() < 2) {
      continue;
    }

    mux_regions.push_back(record.members);
  }
  return mux_regions;
}

} // namespace ocudu::detail

pucch_collision_manager::pucch_collision_manager(const cell_configuration& cell_cfg_) :
  cell_cfg(cell_cfg_),
  resources(make_cell_resource_list(cell_cfg)),
  col_matrix(make_collision_matrix(resources)),
  mux_matrix(make_mux_regions_matrix(resources)),
  mux_region_lookup(build_mux_region_lookup(mux_matrix)),
  slots_ctx(get_allocator_ring_size_gt_min(get_max_slot_ul_alloc_delay(cell_cfg.ntn_cs_koffset)),
            {slot_context(cell_cfg)})
{
}

void pucch_collision_manager::slot_indication(slot_point sl_tx)
{
  // If last_sl_ind is not valid (not initialized), then the check sl_tx == last_sl_ind + 1 does not matter.
  ocudu_sanity_check(not last_sl_ind.valid() or sl_tx == last_sl_ind + 1, "Detected a skipped slot");

  // Clear previous slot context.
  slots_ctx[(sl_tx - 1).count()].current_state.reset();
  slots_ctx[(sl_tx - 1).count()].pucch_res_grid.clear();
  // Update last slot indication.
  last_sl_ind = sl_tx;
}

void pucch_collision_manager::stop()
{
  // Clear all slot contexts.
  for (auto& ctx : slots_ctx) {
    ctx.current_state.reset();
    ctx.pucch_res_grid.clear();
  }
  last_sl_ind = {};
}

pucch_collision_manager::alloc_result_t
pucch_collision_manager::alloc_common(cell_slot_resource_grid& ul_res_grid, slot_point sl, r_pucch_t r_pucch)
{
  return alloc(ul_res_grid, sl, r_pucch.value());
}

pucch_collision_manager::alloc_result_t
pucch_collision_manager::alloc_ded(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned cell_res_id)
{
  return alloc(ul_res_grid, sl, get_ded_idx(cell_res_id));
}

bool pucch_collision_manager::free_common(cell_slot_resource_grid& ul_res_grid, slot_point sl, r_pucch_t r_pucch)
{
  return free(ul_res_grid, sl, r_pucch.value());
}

bool pucch_collision_manager::free_ded(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned cell_res_id)
{
  return free(ul_res_grid, sl, get_ded_idx(cell_res_id));
}

unsigned pucch_collision_manager::get_ded_idx(unsigned cell_res_id) const
{
  ocudu_assert(cell_res_id < cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources.size(),
               "Dedicated PUCCH resource index {} exceeds the maximum allowed {}.",
               cell_res_id,
               cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources.size());
  return pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES + cell_res_id;
}

pucch_collision_manager::mux_region_lookup_t
pucch_collision_manager::build_mux_region_lookup(const detail::mux_regions_matrix& mux_matrix)
{
  mux_region_lookup_t lookup;
  for (unsigned mux_region_idx = 0; mux_region_idx != mux_matrix.size(); ++mux_region_idx) {
    const auto& region = mux_matrix[mux_region_idx];
    for (unsigned res_idx = 0; res_idx != region.size(); ++res_idx) {
      if (not region.test(res_idx)) {
        continue;
      }
      ocudu_assert(not lookup.contains(res_idx), "PUCCH resource {} belongs to multiple multiplexing regions", res_idx);
      lookup.emplace(res_idx, mux_region_idx);
    }
  }

  return lookup;
}

pucch_collision_manager::alloc_result_t
pucch_collision_manager::alloc(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned res_idx)
{
  ocudu_sanity_check(sl < last_sl_ind + slots_ctx.size(),
                     "PUCCH resource ring-buffer accessed too far into the future");

  auto& ctx = slots_ctx[sl.count()];

  // Check for PUCCH-to-other UL grant collisions using the resource grids.
  const auto& res = resources[res_idx];
  if (ul_res_grid.collides(res.grants.first_hop, &ctx.pucch_res_grid)) {
    return make_unexpected(alloc_failure_reason::UL_GRANT_COLLISION);
  }
  if (res.grants.second_hop.has_value() and ul_res_grid.collides(*res.grants.second_hop, &ctx.pucch_res_grid)) {
    return make_unexpected(alloc_failure_reason::UL_GRANT_COLLISION);
  }

  // Check for PUCCH-to-PUCCH collisions using the collision matrix.
  const auto& row = col_matrix[res_idx];
  if ((row & ctx.current_state).any()) {
    return make_unexpected(alloc_failure_reason::PUCCH_COLLISION);
  }

  // Allocate the resource.
  ctx.current_state.set(res_idx);

  // Fill grants in ul_res_grid and ctx.pucch_res_grid.
  ul_res_grid.fill(res.grants.first_hop);
  ctx.pucch_res_grid.fill(res.grants.first_hop);
  if (res.grants.second_hop.has_value()) {
    ul_res_grid.fill(*res.grants.second_hop);
    ctx.pucch_res_grid.fill(*res.grants.second_hop);
  }
  return default_success_t();
}

bool pucch_collision_manager::free(cell_slot_resource_grid& ul_res_grid, slot_point sl, unsigned res_idx)
{
  ocudu_sanity_check(sl < last_sl_ind + slots_ctx.size(),
                     "PUCCH resource ring-buffer accessed too far into the future");

  auto& ctx = slots_ctx[sl.count()];
  if (not ctx.current_state.test(res_idx)) {
    // Resource was not allocated.
    return false;
  }
  ctx.current_state.reset(res_idx);

  // Check if any other resource in the same multiplexing region is still allocated.
  // If not, clear the grants in ul_res_grid and ctx.pucch_res_grid.
  if (mux_region_lookup.contains(res_idx)) {
    const auto& mux_region = mux_matrix[mux_region_lookup[res_idx]];
    if ((mux_region & ctx.current_state).any()) {
      return true;
    }
  }

  // Clear grants in ul_res_grid and ctx.pucch_res_grid.
  const auto& res = resources[res_idx];
  ul_res_grid.clear(res.grants.first_hop);
  ctx.pucch_res_grid.clear(res.grants.first_hop);
  if (res.grants.second_hop.has_value()) {
    ul_res_grid.clear(*res.grants.second_hop);
    ctx.pucch_res_grid.clear(*res.grants.second_hop);
  }
  return true;
}
