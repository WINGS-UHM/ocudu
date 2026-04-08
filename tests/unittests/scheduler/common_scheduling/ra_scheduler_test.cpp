// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/scheduler/common_scheduling/csi_rs_scheduler.h"
#include "lib/scheduler/common_scheduling/ra_scheduler.h"
#include "lib/scheduler/common_scheduling/sib1_scheduler.h"
#include "lib/scheduler/support/csi_rs_helpers.h"
#include "sub_scheduler_test_environment.h"
#include "tests/test_doubles/scheduler/cell_config_builder_profiles.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "tests/test_doubles/scheduler/scheduler_result_finder.h"
#include "tests/test_doubles/utils/test_rng.h"
#include "tests/unittests/scheduler/test_utils/dummy_test_components.h"
#include "tests/unittests/scheduler/test_utils/indication_generators.h"
#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include "ocudu/ran/prach/ra_helper.h"
#include "ocudu/scheduler/config/time_domain_resource_helper.h"
#include <algorithm>
#include <gtest/gtest.h>

using namespace ocudu;
using namespace cell_config_builder_profiles;

namespace {

class ra_scheduler_setup : public sub_scheduler_test_environment
{
  static constexpr unsigned CRNTI_RANGE = to_value(rnti_t::MAX_CRNTI) - to_value(rnti_t::MIN_CRNTI);

public:
  ra_scheduler_setup(const sched_cell_configuration_request_message& req, bool sched_csi, bool sched_sib1) :
    ra_scheduler_setup({}, req, sched_csi, sched_sib1)
  {
  }
  ra_scheduler_setup(const scheduler_expert_config&                  sched_cfg_,
                     const sched_cell_configuration_request_message& req,
                     bool                                            sched_csi,
                     bool                                            sched_sib1) :
    sub_scheduler_test_environment(sched_cfg_, req)
  {
    if (sched_csi) {
      csi_rs_sch.emplace(cell_cfg);
    }
    if (sched_sib1) {
      sib1_sch.emplace(cell_cfg, *pdcch_alloc, units::bytes{108});
    }

    rnti_count = test_rng::uniform_int<unsigned>(0, CRNTI_RANGE);
    rnti_inc   = test_rng::uniform_int<unsigned>(1, 5);

    // Run slot once so that the resource grid gets initialized with the initial slot.
    this->run_slot();
    for (unsigned i = 0; i != res_grid.max_dl_slot_alloc_delay - 1; ++i) {
      if (csi_rs_sch.has_value()) {
        csi_rs_sch->run_slot(res_grid[i]);
      }
      if (sib1_sch.has_value()) {
        sib1_sch->run_slot(res_grid[i]);
      }
    }
  }
  ~ra_scheduler_setup() override { this->flush_events(); }

  void do_run_slot() override
  {
    if (csi_rs_sch.has_value()) {
      csi_rs_sch->run_slot(res_grid[res_grid.max_dl_slot_alloc_delay]);
    }
    if (sib1_sch.has_value()) {
      sib1_sch->run_slot(res_grid[res_grid.max_dl_slot_alloc_delay]);
    }
    ra_sch.run_slot(res_grid);
    ASSERT_NO_FATAL_FAILURE(tracker.on_new_result(res_grid[0].slot, res_grid[0].result));
  }

  void handle_rach_indication(const rach_indication_message& ind)
  {
    ra_sch.handle_rach_indication(ind);
    tracker.on_new_rach_ind(ind);
  }

  void handle_crc_indication(const ul_crc_indication& crc)
  {
    ra_sch.handle_crc_indication(crc);
    tracker.on_crc_indication(crc);
  }

  rach_indication_message::preamble create_random_preamble()
  {
    const auto next_rnti = rnti_count + to_value(rnti_t::MIN_CRNTI);

    rach_indication_message::preamble preamble =
        test_helper::create_preamble(test_rng::uniform_int<unsigned>(0, 63), to_rnti(next_rnti));
    preamble.time_advance =
        phy_time_unit::from_seconds(std::uniform_real_distribution<double>{0, 2005e-6}(test_rng::tls_gen()));

    rnti_count = (rnti_count + rnti_inc) % CRNTI_RANGE;
    return preamble;
  }

  rach_indication_message create_rach_indication(unsigned nof_preambles)
  {
    std::vector<rach_indication_message::preamble> preambles;
    for (unsigned i = 0; i != nof_preambles; ++i) {
      preambles.push_back(create_random_preamble());
    }
    return test_helper::create_rach_indication(next_slot_rx(), preambles);
  }

  bool grants_scheduled_in_next_slots(unsigned nof_slots_to_check)
  {
    for (unsigned i = 0; i != nof_slots_to_check; ++i) {
      this->run_slot();

      auto& dl_res = res_grid[0].result.dl;
      auto& ul_res = res_grid[0].result.ul;

      const bool has_ra_dl_pdcch =
          std::any_of(dl_res.dl_pdcchs.begin(), dl_res.dl_pdcchs.end(), [](const pdcch_dl_information& pdcch) {
            return pdcch.dci.type == dci_dl_rnti_config_type::ra_f1_0;
          });
      const bool has_msg3_retx_pdcch =
          std::any_of(dl_res.ul_pdcchs.begin(), dl_res.ul_pdcchs.end(), [](const pdcch_ul_information& pdcch) {
            return pdcch.dci.type == dci_ul_rnti_config_type::tc_rnti_f0_0;
          });
      const bool has_msg3_pusch =
          std::any_of(ul_res.puschs.begin(), ul_res.puschs.end(), [](const ul_sched_info& pusch) {
            return pusch.context.ue_index == INVALID_DU_UE_INDEX;
          });

      if (not dl_res.rar_grants.empty() or has_ra_dl_pdcch or has_msg3_retx_pdcch or has_msg3_pusch) {
        return true;
      }
    }
    return false;
  }

  void handle_crc_for_pending_puschs(bool crc)
  {
    const auto& puschs = this->res_grid[0].result.ul.puschs;
    if (not puschs.empty()) {
      handle_crc_indication(test_helper::create_crc_indication(this->res_grid[0].slot, puschs, crc));
    }
  }

  ra_scheduler                      ra_sch{cell_cfg, *pdcch_alloc, ev_logger, metrics_hdlr};
  std::optional<csi_rs_scheduler>   csi_rs_sch;
  std::optional<sib1_scheduler>     sib1_sch;
  test_helper::ra_scheduler_tracker tracker{cell_cfg};
  unsigned                          rnti_count = 0;
  unsigned                          rnti_inc   = 1;
};

struct test_params {
  frequency_range                        fr;
  unsigned                               min_k;
  std::optional<tdd_ul_dl_config_common> tdd_cfg;
  bool                                   sched_csi_rs = false;
  bool                                   sched_sib1   = false;
};

/// Test suite common to different FRs, duplex modes, k values.
class ra_scheduler_common_test : public ra_scheduler_setup, public ::testing::TestWithParam<test_params>
{
public:
  ra_scheduler_common_test() :
    ra_scheduler_setup(get_sched_req(GetParam()), GetParam().sched_csi_rs, GetParam().sched_sib1)
  {
  }

  static sched_cell_configuration_request_message get_sched_req(const test_params& t_params)
  {
    cell_config_builder_params builder_params =
        create(t_params.tdd_cfg.has_value() ? duplex_mode::TDD : duplex_mode::FDD, t_params.fr);
    builder_params.min_k2               = t_params.min_k;
    builder_params.min_k1               = t_params.min_k;
    builder_params.tdd_ul_dl_cfg_common = t_params.tdd_cfg;
    return sched_config_helper::make_default_sched_cell_configuration_request(builder_params);
  }
};

/// This test verifies that the cell resource grid remains empty when no RACH indications arrive to the RA scheduler.
TEST_P(ra_scheduler_common_test, when_no_rach_indication_received_then_no_rar_allocated)
{
  ASSERT_FALSE(grants_scheduled_in_next_slots(10));
}

TEST_P(ra_scheduler_common_test,
       when_rach_indication_with_single_preamble_received_then_one_rar_and_one_msg3_are_allocated)
{
  // Forward single RACH occasion with multiple preambles.
  rach_indication_message one_rach = create_rach_indication(1);
  handle_rach_indication(one_rach);

  for (unsigned slot_count = 0, max_slot_count = 1000; slot_count < max_slot_count and tracker.nof_msg3_acked() == 0;
       ++slot_count) {
    run_slot();

    ASSERT_EQ(this->res_grid[0].result.dl.ue_grants.size(), 0);
    ASSERT_EQ(this->res_grid[0].result.dl.ul_pdcchs.size(), 0);
    handle_crc_for_pending_puschs(true);
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(10));
  ASSERT_EQ(tracker.nof_ra_dl_pdcchs(), 1);
  ASSERT_EQ(tracker.nof_rars(), 1);
  ASSERT_EQ(tracker.nof_msg3_acked(), 1);
}

/// This test verifies the correct scheduling of a RAR and Msg3 when multiple RACH Preambles are received, all in a
/// single RACH occasion.
/// The scheduler is expected to allocate one or more RARs (all with the same RA-RNTI), and containing one or multiple
/// Msg3s.
TEST_P(ra_scheduler_common_test,
       when_rach_indication_with_multiple_preambles_received_then_multiple_msg3s_are_allocated)
{
  // Forward single RACH occasion with multiple preambles.
  const unsigned          nof_preambles = test_rng::uniform_int<unsigned>(2, 8);
  rach_indication_message one_rach      = create_rach_indication(nof_preambles);
  handle_rach_indication(one_rach);

  unsigned         max_msg3_per_rar = 0;
  std::set<rnti_t> ra_rntis;
  for (unsigned slot_count = 0, max_slots = 1000; slot_count != max_slots and tracker.nof_msg3_acked() < nof_preambles;
       ++slot_count) {
    run_slot();

    const auto& rars = this->res_grid[0].result.dl.rar_grants;
    for (const rar_information& rar : rars) {
      ra_rntis.emplace(rar.pdsch_cfg.rnti);
      max_msg3_per_rar = std::max<unsigned>(max_msg3_per_rar, rar.grants.size());
    }

    handle_crc_for_pending_puschs(true);
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(10));
  ASSERT_EQ(ra_rntis.size(), 1) << "Only one RA-RNTI was expected";
  ASSERT_GT(max_msg3_per_rar, 1) << "The RA scheduler should try to schedule multiple Msg3s per RAR";
  ASSERT_EQ(tracker.nof_ra_dl_pdcchs(), tracker.nof_rars());
  ASSERT_LE(tracker.nof_rars(), one_rach.occasions[0].preambles.size());
  ASSERT_EQ(tracker.nof_msg3_newtxs(), one_rach.occasions[0].preambles.size());
  ASSERT_EQ(tracker.nof_msg3_acked(), tracker.nof_msg3_newtxs());
}

/// This test verifies the correct scheduling of a RAR and Msg3 when multiple RACH Preambles are received, each in a
/// different PRACH occasion.
/// The scheduler is expected to allocate several RARs (with different RA-RNTIs), each composed by one Msg3.
TEST_P(ra_scheduler_common_test, when_rach_indication_with_multiple_occasions_received_then_multiple_rars_are_allocated)
{
  auto                    nof_occasions = test_rng::uniform_int<unsigned>(1, MAX_PRACH_OCCASIONS_PER_SLOT);
  rach_indication_message rach_ind      = create_rach_indication(0);
  for (unsigned i = 0; i != nof_occasions; ++i) {
    rach_ind.occasions.emplace_back();
    rach_ind.occasions.back().start_symbol    = 0;
    rach_ind.occasions.back().frequency_index = i;
    rach_ind.occasions.back().preambles.emplace_back(create_random_preamble());
  }
  handle_rach_indication(rach_ind);

  for (unsigned slot_count = 0, max_slots = 1000; slot_count != max_slots and tracker.nof_msg3_acked() < nof_occasions;
       ++slot_count) {
    run_slot();

    handle_crc_for_pending_puschs(true);
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(10));
  ASSERT_EQ(tracker.nof_ra_dl_pdcchs(), tracker.nof_rars());
  ASSERT_LE(tracker.nof_rars(), nof_occasions);
  ASSERT_EQ(tracker.nof_msg3_newtxs(), tracker.nof_rars());
  ASSERT_EQ(tracker.nof_msg3_acked(), tracker.nof_msg3_newtxs());
}

TEST_P(ra_scheduler_common_test, when_crc_is_ko_then_msg3_retx_is_scheduled)
{
  // Forward single RACH occasion with multiple preambles.
  const unsigned          nof_preambles = test_rng::uniform_int<unsigned>(2, 8);
  rach_indication_message one_rach      = create_rach_indication(nof_preambles);
  handle_rach_indication(one_rach);

  // Run until Msg3s get NACKed.
  for (unsigned slot_count = 0, max_slots = 1000; slot_count != max_slots and tracker.nof_msg3_newtxs() < nof_preambles;
       ++slot_count) {
    run_slot();
    handle_crc_for_pending_puschs(false);
  }

  // ACK following Msg3s.
  ASSERT_EQ(tracker.nof_msg3_acked(), 0);
  for (unsigned slot_count = 0, max_slots = 1000; slot_count != max_slots and tracker.nof_msg3_acked() < nof_preambles;
       ++slot_count) {
    run_slot();
    handle_crc_for_pending_puschs(true);
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(20));
  ASSERT_EQ(tracker.nof_msg3_acked(), nof_preambles);
  ASSERT_EQ(tracker.nof_msg3_newtxs(), nof_preambles);
  ASSERT_GE(tracker.nof_msg3_retxs(), nof_preambles);
}

using tdd_fr1_30khz = tdd_pattern_profile_fr1_30khz;
INSTANTIATE_TEST_SUITE_P(
    ra_scheduler,
    ra_scheduler_common_test,
    ::testing::Values(
        // FR1, FDD.
        test_params{frequency_range::FR1, 2},
        test_params{frequency_range::FR1, 4},
        test_params{frequency_range::FR1, 4, std::nullopt, true},
        // FR1, TDD.
        test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_fr1_30khz::DDDDDDDSUU)},
        test_params{frequency_range::FR1, 4, create_tdd_pattern(tdd_fr1_30khz::DDDDDDDSUU)},
        test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_fr1_30khz::DDDSU)},
        test_params{frequency_range::FR1, 1, create_tdd_pattern(tdd_fr1_30khz::DSUU)},
        test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_fr1_30khz::DDDDDDDSUU), true},
        test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_fr1_30khz::DDDDDDDSUU), true, true},
        test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_pattern_profile_fr1_30khz::DDDSU), true, true},
        // FR2, TDD.
        test_params{frequency_range::FR2, 1, create_tdd_pattern(tdd_pattern_profile_fr2_120khz::DDDSU)},
        test_params{frequency_range::FR2, 1, create_tdd_pattern(tdd_pattern_profile_fr2_120khz::DDDSU), true}));

class ra_scheduler_failed_rar_test : public ra_scheduler_setup, public ::testing::TestWithParam<test_params>
{
public:
  ra_scheduler_failed_rar_test() :
    ra_scheduler_setup(get_sched_req(GetParam()), GetParam().sched_csi_rs, GetParam().sched_sib1)
  {
  }

  static sched_cell_configuration_request_message get_sched_req(const test_params& t_params)
  {
    cell_config_builder_params builder_params =
        create(t_params.tdd_cfg.has_value() ? duplex_mode::TDD : duplex_mode::FDD, t_params.fr);
    builder_params.min_k2               = t_params.min_k;
    builder_params.min_k1               = t_params.min_k;
    builder_params.tdd_ul_dl_cfg_common = t_params.tdd_cfg;
    return sched_config_helper::make_default_sched_cell_configuration_request(builder_params);
  }

  void fill_dl_rbs(unsigned lookahead_slots)
  {
    if (cell_cfg.is_dl_enabled(res_grid[lookahead_slots].slot)) {
      const grant_info grant{
          cell_cfg.scs_common(),
          ofdm_symbol_range{cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common.coreset0->duration(),
                            NOF_OFDM_SYM_PER_SLOT_NORMAL_CP},
          crb_interval{0, cell_cfg.nof_dl_prbs}};
      res_grid[lookahead_slots].dl_res_grid.fill(grant);
    }
  }
};

TEST_P(ra_scheduler_failed_rar_test, when_no_dl_rbs_available_then_rar_is_not_scheduled)
{
  // Enqueue RACH indication.
  handle_rach_indication(create_rach_indication(test_rng::uniform_int<unsigned>(1, MAX_PREAMBLES_PER_PRACH_OCCASION)));

  // Mark all DL RBs of the first lookahead_dl_rbs slots as busy.
  const unsigned lookahead_dl_rbs = 16;
  for (unsigned i = 0; i != lookahead_dl_rbs; ++i) {
    fill_dl_rbs(i);
  }

  // Keep marking DL RBs as busy until RAR expiry.
  for (unsigned i = 0, nof_slots = 100; i != nof_slots and tracker.has_pending_ra(); ++i) {
    fill_dl_rbs(lookahead_dl_rbs);
    run_slot();
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(lookahead_dl_rbs * 2))
      << "After RAR window expiry, nothing should be scheduled";
  ASSERT_EQ(tracker.nof_ra_dl_pdcchs(), 0);
  ASSERT_EQ(tracker.nof_rars(), 0);
}

TEST_P(ra_scheduler_failed_rar_test, when_no_ul_rbs_available_then_rar_is_not_scheduled)
{
  // Enqueue RACH indication.
  handle_rach_indication(create_rach_indication(test_rng::uniform_int<unsigned>(1, MAX_PREAMBLES_PER_PRACH_OCCASION)));

  // Mark all UL RBs of the first lookahead_ul_rbs slots as busy.
  const unsigned   lookahead_ul_rbs = res_grid.max_ul_slot_alloc_delay;
  const grant_info marked_res{cell_cfg.scs_common(),
                              ofdm_symbol_range{0, NOF_OFDM_SYM_PER_SLOT_NORMAL_CP},
                              crb_interval{0, cell_cfg.nof_ul_prbs}};
  for (unsigned i = 0; i != lookahead_ul_rbs; ++i) {
    if (cell_cfg.is_ul_enabled(res_grid[i].slot)) {
      res_grid[i].ul_res_grid.fill(marked_res);
    }
  }

  // Keep marking UL RBs as busy until RAR expiry.
  for (unsigned i = 0, nof_slots = 100; i != nof_slots and tracker.has_pending_ra(); ++i) {
    if (cell_cfg.is_ul_enabled(res_grid[lookahead_ul_rbs].slot)) {
      res_grid[lookahead_ul_rbs].ul_res_grid.fill(marked_res);
    }
    run_slot();
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(lookahead_ul_rbs * 2))
      << "After RAR window expiry, nothing should be scheduled";
  ASSERT_EQ(tracker.nof_ra_dl_pdcchs(), 0);
  ASSERT_EQ(tracker.nof_rars(), 0);
}

TEST_P(ra_scheduler_failed_rar_test, when_rbs_available_then_new_rars_can_be_scheduled)
{
  // Enqueue first RACH (which is going to fail to be allocated).
  auto first_rach = create_rach_indication(test_rng::uniform_int<unsigned>(1, MAX_PREAMBLES_PER_PRACH_OCCASION));
  handle_rach_indication(first_rach);

  // Let RAR window expire.
  const unsigned lookahead_dl_rbs = 16;
  for (unsigned i = 0; i != lookahead_dl_rbs; ++i) {
    fill_dl_rbs(i);
  }
  for (unsigned i = 0, nof_slots = 100; i != nof_slots and tracker.has_pending_ra(); ++i) {
    fill_dl_rbs(lookahead_dl_rbs);
    run_slot();
  }
  ASSERT_FALSE(grants_scheduled_in_next_slots(lookahead_dl_rbs));
  ASSERT_EQ(tracker.nof_rars(), 0);

  // New RACH is scheduled, but this time it should be possible to schedule new RARs.
  auto second_rach = create_rach_indication(test_rng::uniform_int<unsigned>(1, 8));
  handle_rach_indication(second_rach);
  for (unsigned i = 0, nof_slots = 100; i != nof_slots and tracker.has_pending_ra(); ++i) {
    run_slot();
    handle_crc_for_pending_puschs(true);
  }
  ASSERT_GT(tracker.nof_rars(), 0);
  ASSERT_EQ(tracker.nof_rars(), tracker.nof_ra_dl_pdcchs());
  ASSERT_EQ(tracker.nof_msg3_newtxs(), second_rach.occasions[0].preambles.size());
  ASSERT_EQ(tracker.nof_msg3_newtxs(), tracker.nof_msg3_acked());
}

INSTANTIATE_TEST_SUITE_P(ra_scheduler,
                         ra_scheduler_failed_rar_test,
                         ::testing::Values(
                             // FR1, FDD.
                             test_params{frequency_range::FR1, 2},
                             // FR1, TDD.
                             test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_fr1_30khz::DDDDDDDSUU)},
                             test_params{frequency_range::FR1, 2, create_tdd_pattern(tdd_fr1_30khz::DDDSU)}));

} // namespace
