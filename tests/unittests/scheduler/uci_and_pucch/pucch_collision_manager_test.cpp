/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/scheduler/cell/resource_grid.h"
#include "lib/scheduler/config/cell_configuration.h"
#include "lib/scheduler/pucch_scheduling/pucch_collision_manager.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include "ocudu/scheduler/config/scheduler_expert_config_factory.h"
#include <gtest/gtest.h>

using namespace ocudu;

// static cell_configuration make_test_cell_configuration(const std::vector<pucch_resource>& ded_res               = {},
//                                                        unsigned                           pucch_resource_common = 11)
// {
//   const auto expert_cfg         = config_helpers::make_default_scheduler_expert_config();
//   auto       sched_req          = sched_config_helper::make_default_sched_cell_configuration_request();
//   // sched_req.ded_pucch_resources = ded_res;
//   sched_req.ran.ul_cfg_common.init_ul_bwp.pucch_cfg_common.value().pucch_resource_common = pucch_resource_common;
//   return cell_configuration{expert_cfg, sched_req};
// }
//
// static unsigned ded_idx(unsigned cell_res_id)
// {
//   return pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES + cell_res_id;
// }
//
// TEST(pucch_collision_manager_test, common_resources_dont_collide_with_each_other)
// {
//   static constexpr unsigned nof_common_res_sets = 16;
//   for (unsigned pucch_resource_common = 0; pucch_resource_common != nof_common_res_sets; ++pucch_resource_common) {
//     const auto cell_cfg   = make_test_cell_configuration({}, pucch_resource_common);
//     const auto col_matrix = detail::make_collision_matrix(detail::make_cell_resource_list(cell_cfg));
//
//     for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
//       const auto& row = col_matrix[r_pucch];
//       ASSERT_EQ(1U, row.count());
//       ASSERT_TRUE(row.test(r_pucch));
//     }
//   }
// }
//
// // For reference, these are the common PUCCH resources with the default configuration:
// //
// // | r_pucch | Format | Mux index (CS) | 1st hop (RB,sym) | 2nd hop (RB,sym) |
// // |---------|--------|----------------|------------------|------------------|
// // | 0       | F1     | 0              | [0, 1) [0, 7)    | [51, 52) [7, 14) |
// // | 1       | F1     | 6              | [0, 1) [0, 7)    | [51, 52) [7, 14) |
// // | 2       | F1     | 0              | [1, 2) [0, 7)    | [50, 51) [7, 14) |
// // | 3       | F1     | 6              | [1, 2) [0, 7)    | [50, 51) [7, 14) |
// // | 4       | F1     | 0              | [2, 3) [0, 7)    | [49, 50) [7, 14) |
// // | 5       | F1     | 6              | [2, 3) [0, 7)    | [49, 50) [7, 14) |
// // | 6       | F1     | 0              | [3, 4) [0, 7)    | [48, 49) [7, 14) |
// // | 7       | F1     | 6              | [3, 4) [0, 7)    | [48, 49) [7, 14) |
// // | 8       | F1     | 0              | [51, 52) [0, 7)  | [0, 1) [7, 14)   |
// // | 9       | F1     | 6              | [51, 52) [0, 7)  | [0, 1) [7, 14)   |
// // | 10      | F1     | 0              | [50, 51) [0, 7)  | [1, 2) [7, 14)   |
// // | 11      | F1     | 6              | [50, 51) [0, 7)  | [1, 2) [7, 14)   |
// // | 12      | F1     | 0              | [49, 50) [0, 7)  | [2, 3) [7, 14)   |
// // | 13      | F1     | 6              | [49, 50) [0, 7)  | [2, 3) [7, 14)   |
// // | 14      | F1     | 0              | [48, 49) [0, 7)  | [3, 4) [7, 14)   |
// // | 15      | F1     | 6              | [48, 49) [0, 7)  | [3, 4) [7, 14)   |
// // |---------|--------|----------------|------------------|------------------|
// TEST(pucch_collision_manager_test, resources_of_different_formats_collide_if_they_overlap_in_time_freq)
// {
//   const auto cell_cfg   = make_test_cell_configuration({
//       pucch_resource{
//             .res_id           = {0, 0},
//             .starting_prb     = 0,
//             .second_hop_prb   = std::nullopt,
//             .nof_symbols      = 14,
//             .starting_sym_idx = 0,
//             .format           = pucch_format::FORMAT_2,
//             .format_params    = pucch_format_2_3_cfg{.nof_prbs = 1},
//       },
//   });
//   const auto col_matrix = detail::make_collision_matrix(detail::make_cell_resource_list(cell_cfg));
//
//   // These common resources don't collide with the dedicated resource because:
//   //  - No overlap in time/freq.
//   for (unsigned r_pucch = 2; r_pucch != 8; ++r_pucch) {
//     ASSERT_EQ(1U, col_matrix[r_pucch].count());
//   }
//   for (unsigned r_pucch = 10; r_pucch != 16; ++r_pucch) {
//     ASSERT_EQ(1U, col_matrix[r_pucch].count());
//   }
//
//   // These common resources do collide with the dedicated resource because:
//   //  - Overlap in time/freq.
//   //  - Different formats.
//   for (unsigned r_pucch : {0, 1, 8, 9}) {
//     const auto& row = col_matrix[r_pucch];
//     ASSERT_EQ(2U, row.count());
//     ASSERT_TRUE(row.test(ded_idx(0U)));
//   }
// }
//
// TEST(pucch_collision_manager_test, multiplexed_resources_collide_if_different_time_freq_grants)
// {
//   const auto cell_cfg   = make_test_cell_configuration({
//       pucch_resource{
//             .res_id           = {0, 0},
//             .starting_prb     = 0,
//             .second_hop_prb   = std::nullopt,
//             .nof_symbols      = 14,
//             .starting_sym_idx = 0,
//             .format           = pucch_format::FORMAT_1,
//             .format_params    = pucch_format_1_cfg{.initial_cyclic_shift = 0, .time_domain_occ = 0},
//       },
//   });
//   const auto col_matrix = detail::make_collision_matrix(detail::make_cell_resource_list(cell_cfg));
//
//   // These common resources don't collide with the dedicated resource because:
//   //  - No overlap in time/freq.
//   for (unsigned r_pucch = 2; r_pucch != 8; ++r_pucch) {
//     ASSERT_EQ(1U, col_matrix[r_pucch].count());
//   }
//   for (unsigned r_pucch = 10; r_pucch != 16; ++r_pucch) {
//     ASSERT_EQ(1U, col_matrix[r_pucch].count());
//   }
//
//   // These common resources do collide with the dedicated resource because:
//   //  - Overlap in time/freq.
//   //  - Different time/freq grants.
//   for (unsigned r_pucch : {0, 1, 8, 9}) {
//     const auto& row = col_matrix[r_pucch];
//     ASSERT_EQ(2U, row.count());
//     ASSERT_TRUE(row.test(ded_idx(0U)));
//   }
// }
//
// TEST(pucch_collision_manager_test, f0_multiplexed_resources_dont_collide)
// {
//   std::vector<pucch_resource> ded_res_list;
//   for (uint8_t ics = 0; ics != pucch_constants::format0_initial_cyclic_shift_range.stop(); ++ics) {
//     ded_res_list.push_back(pucch_resource{
//         .res_id           = {ics, ics},
//         .starting_prb     = 0,
//         .second_hop_prb   = std::nullopt,
//         .nof_symbols      = 2,
//         .starting_sym_idx = 0,
//         .format           = pucch_format::FORMAT_0,
//         .format_params    = pucch_format_0_cfg{.initial_cyclic_shift = ics},
//     });
//   }
//   const auto cell_cfg   = make_test_cell_configuration(ded_res_list);
//   const auto col_matrix = detail::make_collision_matrix(detail::make_cell_resource_list(cell_cfg));
//
//   // Check that no dedicated resources collide with each other.
//   for (unsigned i = 0; i != ded_res_list.size(); ++i) {
//     const auto& row = col_matrix[ded_idx(i)];
//     for (unsigned j = i + 1; j != ded_res_list.size(); ++j) {
//       ASSERT_FALSE(row.test(ded_idx(j)));
//     }
//   }
// }
//
// TEST(pucch_collision_manager_test, f1_multiplexed_resources_dont_collide)
// {
//   std::vector<pucch_resource> ded_res_list;
//   for (uint8_t ics = 0; ics != pucch_constants::format1_initial_cyclic_shift_range.stop(); ++ics) {
//     for (uint8_t occ = 0; occ != pucch_constants::format1_time_domain_occ_range.stop(); ++occ) {
//       ded_res_list.push_back(pucch_resource{
//           .res_id           = {static_cast<unsigned>(ded_res_list.size()),
//           static_cast<unsigned>(ded_res_list.size())}, .starting_prb     = 0, .second_hop_prb   = std::nullopt,
//           .nof_symbols      = 14,
//           .starting_sym_idx = 0,
//           .format           = pucch_format::FORMAT_1,
//           .format_params    = pucch_format_1_cfg{.initial_cyclic_shift = ics, .time_domain_occ = occ},
//       });
//     }
//   }
//   const auto cell_cfg   = make_test_cell_configuration(ded_res_list);
//   const auto col_matrix = detail::make_collision_matrix(detail::make_cell_resource_list(cell_cfg));
//
//   // Check that no dedicated resources collide with each other.
//   for (unsigned i = 0; i != ded_res_list.size(); ++i) {
//     const auto& row = col_matrix[ded_idx(i)];
//     for (unsigned j = i + 1; j != ded_res_list.size(); ++j) {
//       ASSERT_FALSE(row.test(ded_idx(j)));
//     }
//   }
// }
//
// TEST(pucch_collision_manager_test, f4_multiplexed_resources_dont_collide)
// {
//   std::vector<pucch_resource> ded_res_list;
//   for (unsigned occ = 0; occ != 4; ++occ) {
//     ded_res_list.push_back(pucch_resource{
//         .res_id           = {occ, occ},
//         .starting_prb     = 0,
//         .second_hop_prb   = std::nullopt,
//         .nof_symbols      = 14,
//         .starting_sym_idx = 0,
//         .format           = pucch_format::FORMAT_4,
//         .format_params =
//             pucch_format_4_cfg{.occ_length = pucch_f4_occ_len::n4, .occ_index = static_cast<pucch_f4_occ_idx>(occ)},
//     });
//   }
//   const auto cell_cfg   = make_test_cell_configuration(ded_res_list);
//   const auto col_matrix = detail::make_collision_matrix(detail::make_cell_resource_list(cell_cfg));
//
//   // Check that no dedicated resources collide with each other.
//   for (unsigned i = 0; i != ded_res_list.size(); ++i) {
//     const auto& row = col_matrix[ded_idx(i)];
//     for (unsigned j = i + 1; j != ded_res_list.size(); ++j) {
//       ASSERT_FALSE(row.test(ded_idx(j)));
//     }
//   }
// }
//
// TEST(pucch_collision_manager_test, check_mux_regions_count_for_common_resources)
// {
//   // Return the sizes of the expected mux regions according to the number of CS available for a given common
//   resource. auto expected_regions_from_number_of_cs = [](unsigned nof_cs) -> std::vector<unsigned> {
//     switch (nof_cs) {
//       case 2:
//         return {2, 2, 2, 2, 2, 2, 2, 2};
//       case 3:
//         return {3, 3, 2, 3, 3, 2};
//       case 4:
//         return {4, 4, 4, 4};
//       default:
//         ocudu_assertion_failure("Unexpected number of cyclic shifts for common PUCCH resources");
//         return {};
//     }
//   };
//
//   static constexpr std::array<unsigned, 16> pucch_common_res_nof_cs{2, 3, 3, 2, 4, 4, 4, 2, 4, 4, 4, 2, 4, 4, 4, 4};
//
//   for (unsigned pucch_resource_common = 0; pucch_resource_common != 16; ++pucch_resource_common) {
//     const auto              cell_cfg = make_test_cell_configuration({}, pucch_resource_common);
//     pucch_collision_manager col_manager(cell_cfg);
//     const auto              mux_matrix = detail::make_mux_regions_matrix(detail::make_cell_resource_list(cell_cfg));
//
//     const unsigned nof_cs                = pucch_common_res_nof_cs[pucch_resource_common];
//     const auto     expected_region_sizes = expected_regions_from_number_of_cs(nof_cs);
//
//     unsigned r_pucch = 0;
//     ASSERT_EQ(expected_region_sizes.size(), mux_matrix.size());
//     for (unsigned i = 0; i != mux_matrix.size(); ++i) {
//       ASSERT_EQ(expected_region_sizes[i], mux_matrix[i].count());
//
//       // Check the mux region row has the correct resources set.
//       for (unsigned j = 0; j != expected_region_sizes[i]; ++j) {
//         ASSERT_TRUE(mux_matrix[i].test(r_pucch + j));
//       }
//
//       r_pucch += expected_region_sizes[i];
//     }
//   }
// }
//
// TEST(pucch_collision_manager_test, handles_max_dedicated_resources_with_unique_regions)
// {
//   auto cell_cfg = make_test_cell_configuration();
//
//   std::vector<pucch_resource> ded_res_list;
//   ded_res_list.reserve(pucch_constants::MAX_NOF_CELL_DED_RESOURCES);
//
//   // Generate a list with the maximum number of dedicated PUCCH resources in a way that all resources belongs to a
//   // different multiplexing region.
//   for (unsigned sym = 0, prb = 0; ded_res_list.size() != pucch_constants::MAX_NOF_CELL_DED_RESOURCES;) {
//     const unsigned res_idx = ded_res_list.size();
//     ded_res_list.push_back(pucch_resource{
//         .res_id           = {res_idx, res_idx},
//         .starting_prb     = prb,
//         .second_hop_prb   = std::nullopt,
//         .nof_symbols      = 1,
//         .starting_sym_idx = static_cast<uint8_t>(sym),
//         .format           = pucch_format::FORMAT_2,
//         .format_params    = pucch_format_2_3_cfg{.nof_prbs = 1},
//     });
//
//     // By using different starting symbols and PRBs for each resource, we ensure that all resources have unique
//     // time-frequency allocations, and thus belong to different multiplexing regions.
//     // Using Format 2 ensures they are not multiplexed with the common resources either.
//     ++sym;
//     if (sym == NOF_OFDM_SYM_PER_SLOT_NORMAL_CP) {
//       sym = 0;
//       ++prb;
//       if (prb == cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length()) {
//         // No more PRBs available. This should not happen.
//         break;
//       }
//     }
//   }
//   ASSERT_EQ(ded_res_list.size(), pucch_constants::MAX_NOF_CELL_DED_RESOURCES)
//       << "Failed to create the maximum number of dedicated PUCCH resources for the test";
//
//   // Overwrite the cell configuration with the dedicated resources.
//   cell_cfg.ded_pucch_resources = ded_res_list;
//   const auto mux_matrix        = detail::make_mux_regions_matrix(detail::make_cell_resource_list(cell_cfg));
//   // None of the dedicated resources should be part of a multiplexing region.
//   ASSERT_EQ(8U, mux_matrix.size());
// }
//
// class pucch_collision_manager_rg_test : public ::testing::Test
// {
// protected:
//   pucch_collision_manager_rg_test() :
//     // Dedicated resource needed for testing PUCCH-to-PUCCH collisions.
//     ded_res({
//         .res_id           = {0, 0},
//         .starting_prb     = 0,
//         .second_hop_prb   = std::nullopt,
//         .nof_symbols      = 14,
//         .starting_sym_idx = 0,
//         .format           = pucch_format::FORMAT_1,
//         .format_params    = pucch_format_1_cfg{.initial_cyclic_shift = 0, .time_domain_occ = 0},
//     }),
//     cell_cfg(make_test_cell_configuration({ded_res})),
//     common_grants({
//         make_common_grants(0, 51),
//         make_common_grants(0, 51),
//         make_common_grants(1, 50),
//         make_common_grants(1, 50),
//         make_common_grants(2, 49),
//         make_common_grants(2, 49),
//         make_common_grants(3, 48),
//         make_common_grants(3, 48),
//         make_common_grants(51, 0),
//         make_common_grants(51, 0),
//         make_common_grants(50, 1),
//         make_common_grants(50, 1),
//         make_common_grants(49, 2),
//         make_common_grants(49, 2),
//         make_common_grants(48, 3),
//         make_common_grants(48, 3),
//     }),
//     ded_grant(grant_info(cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.scs,
//                          ofdm_symbol_range::start_and_len(0, 14),
//                          crb_interval::start_and_len(0, 1))),
//     col_manager(cell_cfg),
//     slot_alloc(cell_cfg),
//     sl(0, 0)
//   {
//     col_manager.slot_indication(sl);
//     slot_alloc.slot_indication(sl);
//   }
//
//   void run_slot()
//   {
//     ++sl;
//     col_manager.slot_indication(sl);
//     slot_alloc.slot_indication(sl);
//   }
//
//   const pucch_resource                                    ded_res;
//   const cell_configuration                                cell_cfg;
//   const std::array<std::pair<grant_info, grant_info>, 16> common_grants;
//   const grant_info                                        ded_grant;
//
//   pucch_collision_manager      col_manager;
//   cell_slot_resource_allocator slot_alloc;
//   slot_point                   sl;
//
// private:
//   // Helper to compute common resource grants.
//   std::pair<grant_info, grant_info> make_common_grants(unsigned first_hop_crb, unsigned second_hop_crb) const
//   {
//     return {grant_info{cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.scs,
//                        ofdm_symbol_range::start_and_len(0, 7),
//                        crb_interval::start_and_len(first_hop_crb, 1)},
//             grant_info{cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.scs,
//                        ofdm_symbol_range::start_and_len(7, 7),
//                        crb_interval::start_and_len(second_hop_crb, 1)}};
//   }
// };
//
// static void check_rg_has_expected_grants(const cell_slot_resource_grid& ul_res_grid,
//                                          const std::vector<grant_info>& expected_grants)
// {
//   cell_slot_resource_grid expected_rg = ul_res_grid;
//   expected_rg.clear();
//
//   for (const auto& grant : expected_grants) {
//     ASSERT_TRUE(ul_res_grid.all_set(grant));
//     expected_rg.fill(grant);
//   }
//
//   for (subcarrier_spacing scs : expected_rg.active_scs()) {
//     ASSERT_EQ(ul_res_grid.get_carrier_res_grid(scs), expected_rg.get_carrier_res_grid(scs));
//   }
// }
//
// TEST_F(pucch_collision_manager_rg_test, alloc_fills_grants_in_ul_res_grid)
// {
//   // Allocate all common resources one by one.
//   std::vector<grant_info> expected_grants;
//   for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
//     ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch).has_value());
//
//     // Check that only the expected grants were written to the resource grid.
//     expected_grants.push_back(common_grants[r_pucch].first);
//     expected_grants.push_back(common_grants[r_pucch].second);
//     check_rg_has_expected_grants(slot_alloc.ul_res_grid, expected_grants);
//   }
//
//   // Advance to the next slot.
//   run_slot();
//
//   // Allocate the dedicated resource.
//   ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
//
//   // Check that only the expected grants were written to the resource grid.
//   check_rg_has_expected_grants(slot_alloc.ul_res_grid, {ded_grant});
// }
//
// TEST_F(pucch_collision_manager_rg_test, alloc_fails_if_ul_res_grid_occupied)
// {
//   // Helper to test UL grant collision.
//   auto expect_ul_grant_collision = [&](grant_info grant, auto allocator) {
//     // Fill the resource grid with the conflicting grant.
//     slot_alloc.ul_res_grid.fill(grant);
//
//     // Try the allocation. It should fail with an UL grant collision.
//     auto res = allocator();
//     ASSERT_FALSE(res.has_value());
//     ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::UL_GRANT_COLLISION, res.error());
//
//     // Check that the resource grid was not modified.
//     check_rg_has_expected_grants(slot_alloc.ul_res_grid, {grant});
//
//     // Clear the resource grid.
//     slot_alloc.ul_res_grid.clear(grant);
//   };
//
//   // Simulate UL grant collision when allocating common PUCCH resources.
//   for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
//     // Simulate a collision with the first hop.
//     expect_ul_grant_collision(common_grants[r_pucch].first,
//                               [&]() { return col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch); });
//
//     // Simulate a collision with the second hop.
//     expect_ul_grant_collision(common_grants[r_pucch].second,
//                               [&]() { return col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch); });
//   }
//
//   // Simulate UL grant collision when allocating the dedicated PUCCH resource.
//   expect_ul_grant_collision(ded_grant, [&]() { return col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0); });
// }
//
// TEST_F(pucch_collision_manager_rg_test, alloc_fails_if_pucch_collision)
// {
//   // Note: Common Res 0 collides with the dedicated resource for the tested configuration.
//
//   // First common, then dedicated.
//   ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 0).has_value());
//   ASSERT_FALSE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
//   ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::PUCCH_COLLISION,
//             col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).error());
//
//   // Advance to the next slot.
//   run_slot();
//
//   // First dedicated, then common.
//   ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
//   ASSERT_FALSE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 0).has_value());
//   ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::PUCCH_COLLISION,
//             col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).error());
// }
//
// TEST_F(pucch_collision_manager_rg_test, free_clears_grants_in_ul_res_grid)
// {
//   // Allocate and free all common resources one by one.
//   for (unsigned r_pucch = 0; r_pucch != pucch_constants::MAX_NOF_CELL_COMMON_PUCCH_RESOURCES; ++r_pucch) {
//     ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, r_pucch).has_value());
//     ASSERT_TRUE(col_manager.free_common(slot_alloc.ul_res_grid, sl, r_pucch));
//
//     check_rg_has_expected_grants(slot_alloc.ul_res_grid, {});
//   }
//
//   // Allocate and free the dedicated resource.
//   ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
//   ASSERT_TRUE(col_manager.free_ded(slot_alloc.ul_res_grid, sl, 0));
//
//   check_rg_has_expected_grants(slot_alloc.ul_res_grid, {});
// }
//
// TEST_F(pucch_collision_manager_rg_test, free_doesnt_clear_grants_if_resource_is_being_muxed)
// {
//   // Note: Common Res 0 and 1 are multiplexed together for the tested configuration.
//   ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 0).has_value());
//   ASSERT_TRUE(col_manager.alloc_common(slot_alloc.ul_res_grid, sl, 1).has_value());
//
//   // Note: the grants are the same for both resources.
//   const std::vector<grant_info> expected_grants{
//       common_grants[0].first,
//       common_grants[0].second,
//   };
//   check_rg_has_expected_grants(slot_alloc.ul_res_grid, expected_grants);
//
//   // Free the first resource. Grants should remain because the second resource is still allocated.
//   ASSERT_TRUE(col_manager.free_common(slot_alloc.ul_res_grid, sl, 0));
//   check_rg_has_expected_grants(slot_alloc.ul_res_grid, expected_grants);
//
//   // Free the second resource. Grants should be cleared now.
//   ASSERT_TRUE(col_manager.free_common(slot_alloc.ul_res_grid, sl, 1));
//   check_rg_has_expected_grants(slot_alloc.ul_res_grid, {});
// }
//
// TEST_F(pucch_collision_manager_rg_test, free_doesnt_clear_grants_if_resource_was_not_allocated)
// {
//   // Simulate a non-PUCCH grant over the dedicated resource grant.
//   slot_alloc.ul_res_grid.fill(ded_grant);
//
//   // Try to free the dedicated resource. It should return false and not clear the grant.
//   ASSERT_FALSE(col_manager.free_ded(slot_alloc.ul_res_grid, sl, 0));
//
//   // Check that the resource grid was not modified.
//   check_rg_has_expected_grants(slot_alloc.ul_res_grid, {ded_grant});
// }
//
// TEST_F(pucch_collision_manager_rg_test, slot_indication_clears_pucch_res_grid)
// {
//   for (unsigned i = 0; i != cell_resource_allocator::RING_ALLOCATOR_SIZE; ++i) {
//     // Allocate the dedicated resource.
//     ASSERT_TRUE(col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0).has_value());
//
//     // Advance to the next slot.
//     run_slot();
//   }
//
//   for (unsigned i = 0; i != cell_resource_allocator::RING_ALLOCATOR_SIZE; ++i) {
//     // Simulate a non-PUCCH grant over the dedicated resource grant.
//     slot_alloc.ul_res_grid.fill(ded_grant);
//
//     // Try to allocate the dedicated resource again. It should always fail because of a UL grant collision.
//     // If it doesn't fail, it means the PUCCH resource grid was not cleared on slot indication.
//     auto res = col_manager.alloc_ded(slot_alloc.ul_res_grid, sl, 0);
//     ASSERT_FALSE(res.has_value());
//     ASSERT_EQ(pucch_collision_manager::alloc_failure_reason::UL_GRANT_COLLISION, res.error());
//
//     // Advance to the next slot.
//     run_slot();
//   }
// }
