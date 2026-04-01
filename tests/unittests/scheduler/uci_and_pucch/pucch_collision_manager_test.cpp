// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/scheduler/cell/resource_grid.h"
#include "lib/scheduler/config/cell_configuration.h"
#include "lib/scheduler/config/du_cell_group_config_pool.h"
#include "lib/scheduler/pucch_scheduling/pucch_collision_manager.h"
#include "lib/scheduler/support/pucch/pucch_collision_info.h"
#include "lib/scheduler/support/pucch/pucch_default_resource.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/scheduler/config/pucch_resource_builder_params.h"
#include "ocudu/scheduler/config/scheduler_expert_config_factory.h"
#include "ocudu/scheduler/resource_grid_util.h"
#include <gtest/gtest.h>
#include <memory>

using namespace ocudu;

static std::unique_ptr<du_cell_config_pool>
make_test_cell_config_pool(const pucch_resource_builder_params& ded_resources         = pucch_resource_builder_params{},
                           unsigned                             pucch_resource_common = 11)
{
  const auto expert_cfg                  = config_helpers::make_default_scheduler_expert_config();
  auto       sched_req                   = sched_config_helper::make_default_sched_cell_configuration_request();
  sched_req.ran.init_bwp.pucch.resources = ded_resources;
  sched_req.ran.ul_cfg_common.init_ul_bwp.pucch_cfg_common.value().pucch_resource_common = pucch_resource_common;
  return std::make_unique<du_cell_config_pool>(expert_cfg, sched_req);
}

TEST(pucch_collision_manager_test, check_mux_regions_count_for_common_resources)
{
  // Return the sizes of the expected mux regions according to the number of CS available for a given common resource.
  auto expected_regions_from_number_of_cs = [](unsigned nof_cs) -> std::vector<unsigned> {
    switch (nof_cs) {
      case 2:
        return {2, 2, 2, 2, 2, 2, 2, 2};
      case 3:
        return {3, 3, 2, 3, 3, 2};
      case 4:
        return {4, 4, 4, 4};
      default:
        ocudu_assertion_failure("Unexpected number of cyclic shifts for common PUCCH resources");
        return {};
    }
  };

  for (unsigned pucch_resource_common = 0; pucch_resource_common != 16; ++pucch_resource_common) {
    auto cfg_pool = make_test_cell_config_pool(
        // Disallow multiplexing on dedicated resources so they don't show on the mux regions.
        pucch_resource_builder_params{
            .f0_or_f1_params =
                pucch_f1_params{
                    .nof_cyc_shifts = pucch_nof_cyclic_shifts::no_cyclic_shift,
                    .occ_supported  = false,
                },
        },
        pucch_resource_common);
    const auto& cell_cfg = cfg_pool->cell_cfg();

    pucch_collision_manager col_manager(cell_cfg);
    const auto              mux_matrix = detail::make_mux_regions_matrix(detail::make_cell_resource_list(cell_cfg));

    const auto& default_res           = get_pucch_default_resource(pucch_resource_common, cell_cfg.nof_ul_prbs);
    const auto  expected_region_sizes = expected_regions_from_number_of_cs(default_res.cs_indexes.size());

    unsigned r_pucch = 0;
    ASSERT_EQ(expected_region_sizes.size(), mux_matrix.size());
    for (unsigned i = 0; i != mux_matrix.size(); ++i) {
      ASSERT_EQ(expected_region_sizes[i], mux_matrix[i].count());

      // Check the mux region row has the correct resources set.
      for (unsigned j = 0; j != expected_region_sizes[i]; ++j) {
        ASSERT_TRUE(mux_matrix[i].test(r_pucch + j));
      }

      r_pucch += expected_region_sizes[i];
    }
  }
}

TEST(pucch_collision_manager_test, handles_max_dedicated_resources_with_mux_regions)
{
  auto        cfg_pool   = make_test_cell_config_pool(pucch_resource_builder_params{
               .res_set_0_size           = 8,
               .res_set_1_size           = 8,
               .nof_cell_res_set_configs = 8,
               .nof_cell_sr_resources    = 96,
               .nof_cell_csi_resources   = 32,
               .f0_or_f1_params =
          pucch_f1_params{
                       .nof_syms       = pucch_constants::f1::MIN_NOF_SYMS,
                       .nof_cyc_shifts = pucch_nof_cyclic_shifts::twelve,
                       .occ_supported  = true,
          },
               .f2_or_f3_or_f4_params = pucch_f2_params{.nof_syms = 1, .max_nof_rbs = 1},
  });
  const auto& cell_cfg   = cfg_pool->cell_cfg();
  const auto  mux_matrix = detail::make_mux_regions_matrix(detail::make_cell_resource_list(cell_cfg));
  ASSERT_GT(mux_matrix.size(), 8U);
}

class pucch_collision_manager_rg_test : public ::testing::Test
{
protected:
  pucch_collision_manager_rg_test() :
    cfg_pool(make_test_cell_config_pool()),
    cell_cfg(cfg_pool->cell_cfg()),
    common_infos(make_common_infos()),
    ded_infos(make_ded_infos()),
    col_manager(cell_cfg),
    slot_alloc(cell_cfg),
    sl(0, 0)
  {
    col_manager.slot_indication(sl);
    slot_alloc.slot_indication(sl);
  }

  void run_slot()
  {
    ++sl;
    col_manager.slot_indication(sl);
    slot_alloc.slot_indication(sl);
  }

  std::unique_ptr<du_cell_config_pool>    cfg_pool;
  const cell_configuration&               cell_cfg;
  const std::vector<pucch_collision_info> common_infos;
  const std::vector<pucch_collision_info> ded_infos;

  pucch_collision_manager      col_manager;
  cell_slot_resource_allocator slot_alloc;
  slot_point                   sl;

private:
  std::vector<pucch_collision_info> make_common_infos()
  {
    std::vector<pucch_collision_info> infos;
    const auto                        default_res = get_pucch_default_resource(
        cell_cfg.params.ul_cfg_common.init_ul_bwp.pucch_cfg_common->pucch_resource_common, cell_cfg.nof_ul_prbs);
    for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
      infos.emplace_back(default_res, r_pucch, cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params);
    }
    return infos;
  }

  std::vector<pucch_collision_info> make_ded_infos()
  {
    std::vector<pucch_collision_info> infos;
    infos.reserve(cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources.size());
    for (const auto& resource : cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources) {
      infos.emplace_back(resource, cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params);
    }
    return infos;
  }
};

static void check_rg_has_expected_grants(const cell_slot_resource_grid&           ul_res_grid,
                                         const std::vector<pucch_collision_info>& expected_pucchs)
{
  cell_slot_resource_grid expected_rg = ul_res_grid;
  expected_rg.clear();

  for (const auto& info : expected_pucchs) {
    expected_rg.fill(info.grants.first_hop);
    if (info.grants.second_hop.has_value()) {
      expected_rg.fill(*info.grants.second_hop);
    }
  }

  ASSERT_EQ(ul_res_grid, expected_rg);
}

TEST_F(pucch_collision_manager_rg_test, alloc_fills_grants_in_ul_res_grid)
{
  // Allocate all common resources one by one.
  std::vector<pucch_collision_info> expected_grants;
  for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
    ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch).has_value());
    expected_grants.emplace_back(common_infos[r_pucch]);

    // Check that only the expected grants were written to the resource grid.
    check_rg_has_expected_grants(slot_alloc.ul_res_grid, expected_grants);
  }

  // Advance to the next slot.
  run_slot();
  expected_grants.clear();

  // Allocate all dedicated resources one by one.
  for (unsigned i = 0; i != cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources.size(); ++i) {
    ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, i).has_value());

    // Check that only the expected grants were written to the resource grid.
    expected_grants.emplace_back(ded_infos[i]);
    check_rg_has_expected_grants(slot_alloc.ul_res_grid, expected_grants);
  }
}

TEST_F(pucch_collision_manager_rg_test, alloc_fails_if_ul_res_grid_occupied)
{
  // Helper to test UL grant collision.
  auto expect_ul_grant_collision = [&](grant_info grant, auto allocator) {
    // Fill the resource grid with the conflicting grant.
    slot_alloc.ul_res_grid.fill(grant);

    const auto expected_rg = slot_alloc.ul_res_grid;

    // Try the allocation. It should fail with an UL grant collision.
    auto res = allocator();
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::UL_GRANT_COLLISION, res.error());

    // Check that the resource grid was not modified.
    ASSERT_EQ(slot_alloc.ul_res_grid, expected_rg);

    // Clear the resource grid.
    slot_alloc.ul_res_grid.clear(grant);
  };

  // Simulate UL grant collision when allocating common PUCCH resources.
  for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
    // Simulate a collision with the first hop.
    expect_ul_grant_collision(common_infos[r_pucch].grants.first_hop,
                              [&]() { return col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch); });

    // Simulate a collision with the second hop.
    expect_ul_grant_collision(common_infos[r_pucch].grants.second_hop.value(),
                              [&]() { return col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch); });
  }

  for (unsigned i = 0; i != cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources.size(); ++i) {
    // Simulate UL grant collision when allocating the dedicated PUCCH resource.
    expect_ul_grant_collision(ded_infos[i].grants.first_hop,
                              [&]() { return col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, i); });
  }
}

TEST_F(pucch_collision_manager_rg_test, alloc_fails_if_pucch_collision)
{
  // Note: Common Res 0 collides with the first dedicated resource as both start at the edges of the BWP.

  // First common, then dedicated.
  ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 0).has_value());
  ASSERT_FALSE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
  ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::PUCCH_COLLISION,
            col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).error());

  // Advance to the next slot.
  run_slot();

  // First dedicated, then common.
  ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
  ASSERT_FALSE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 0).has_value());
  ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::PUCCH_COLLISION,
            col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).error());
}

TEST_F(pucch_collision_manager_rg_test, free_clears_grants_in_ul_res_grid)
{
  // Allocate and free all common resources one by one.
  for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
    ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch).has_value());
    ASSERT_TRUE(col_manager.free_common(slot_alloc.ul_res_grid, sl, r_pucch));

    check_rg_has_expected_grants(slot_alloc.ul_res_grid, {});
  }

  // Allocate and free all dedicated resources one by one.
  for (unsigned i = 0; i != cell_cfg.bwp_res[to_bwp_id(0)].ul().pucch.resources.size(); ++i) {
    ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, i).has_value());
    ASSERT_TRUE(col_manager.free_ded(slot_alloc.ul_res_grid, sl, i));

    check_rg_has_expected_grants(slot_alloc.ul_res_grid, {});
  }
}

TEST_F(pucch_collision_manager_rg_test, free_doesnt_clear_grants_if_resource_is_being_muxed)
{
  // Note: Common Res 0 and 1 are multiplexed together for the tested configuration.
  ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 0).has_value());
  ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 1).has_value());

  // Note: the grants are the same for both resources.
  check_rg_has_expected_grants(slot_alloc.ul_res_grid, {common_infos[0]});

  // Free the first resource. Grants should remain because the second resource is still allocated.
  ASSERT_TRUE(col_manager.free_common(slot_alloc.ul_res_grid, sl, 0));
  check_rg_has_expected_grants(slot_alloc.ul_res_grid, {common_infos[0]});

  // Free the second resource. Grants should be cleared now.
  ASSERT_TRUE(col_manager.free_common(slot_alloc.ul_res_grid, sl, 1));
  check_rg_has_expected_grants(slot_alloc.ul_res_grid, {});
}

TEST_F(pucch_collision_manager_rg_test, free_doesnt_clear_grants_if_resource_was_not_allocated)
{
  // Simulate a non-PUCCH grant over the dedicated resource grant.
  slot_alloc.ul_res_grid.fill(ded_infos[0].grants.first_hop);
  if (ded_infos[0].grants.second_hop.has_value()) {
    slot_alloc.ul_res_grid.fill(*ded_infos[0].grants.second_hop);
  }

  // Try to free the dedicated resource. It should return false and not clear the grant.
  ASSERT_FALSE(col_manager.free_ded(slot_alloc.ul_res_grid, sl, 0));

  // Check that the resource grid was not modified.
  check_rg_has_expected_grants(slot_alloc.ul_res_grid, {ded_infos[0]});
}

TEST_F(pucch_collision_manager_rg_test, slot_indication_clears_pucch_res_grid)
{
  const unsigned ring_size = get_allocator_ring_size_gt_min(get_max_slot_ul_alloc_delay(0));
  for (unsigned i = 0; i != ring_size; ++i) {
    // Allocate the dedicated resource.
    ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());

    // Advance to the next slot.
    run_slot();
  }

  for (unsigned i = 0; i != ring_size; ++i) {
    // Simulate a non-PUCCH grant over the dedicated resource grant.
    slot_alloc.ul_res_grid.fill(ded_infos[0].grants.first_hop);
    if (ded_infos[0].grants.second_hop.has_value()) {
      slot_alloc.ul_res_grid.fill(*ded_infos[0].grants.second_hop);
    }

    // Try to allocate the dedicated resource again. It should always fail because of a UL grant collision.
    // If it doesn't fail, it means the PUCCH resource grid was not cleared on slot indication.
    auto res = col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0);
    ASSERT_FALSE(res.has_value());
    ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::UL_GRANT_COLLISION, res.error());

    // Advance to the next slot.
    run_slot();
  }
}
