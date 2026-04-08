// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/scheduler/common_scheduling/ra_scheduler.h"
#include "lib/scheduler/support/csi_rs_helpers.h"
#include "sub_scheduler_test_environment.h"
#include "tests/test_doubles/scheduler/cell_config_builder_profiles.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "tests/test_doubles/scheduler/scheduler_result_finder.h"
#include "tests/test_doubles/utils/test_rng.h"
#include "tests/unittests/scheduler/test_utils/dummy_test_components.h"
#include "tests/unittests/scheduler/test_utils/indication_generators.h"
#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include "ocudu/adt/noop_functor.h"
#include "ocudu/ran/prach/ra_helper.h"
#include "ocudu/ran/resource_allocation/resource_allocation_frequency.h"
#include "ocudu/scheduler/config/time_domain_resource_helper.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <ostream>

using namespace ocudu;
using namespace cell_config_builder_profiles;

namespace {

class ra_scheduler_setup : public sub_scheduler_test_environment
{
  static constexpr unsigned CRNTI_RANGE = to_value(rnti_t::MAX_CRNTI) - to_value(rnti_t::MIN_CRNTI);

public:
  ra_scheduler_setup(const sched_cell_configuration_request_message& req) : ra_scheduler_setup({}, req) {}
  ra_scheduler_setup(const scheduler_expert_config& sched_cfg_, const sched_cell_configuration_request_message& req) :
    sub_scheduler_test_environment(sched_cfg_, req)
  {
    rnti_count = test_rng::uniform_int<unsigned>(0, CRNTI_RANGE);
    rnti_inc   = test_rng::uniform_int<unsigned>(1, 5);

    // Run slot once so that the resource grid gets initialized with the initial slot.
    this->run_slot();
  }
  ~ra_scheduler_setup() override { this->flush_events(); }

  void do_run_slot() override
  {
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
      auto  n =
          std::max({dl_res.rar_grants.size(), dl_res.dl_pdcchs.size(), dl_res.ul_pdcchs.size(), ul_res.puschs.size()});
      if (n > 0) {
        return true;
      }
    }
    return false;
  }

  ra_scheduler                      ra_sch{cell_cfg, *pdcch_alloc, ev_logger, metrics_hdlr};
  test_helper::ra_scheduler_tracker tracker{cell_cfg};
  unsigned                          rnti_count = 0;
  unsigned                          rnti_inc   = 1;
};

/// Parameters used by FDD and TDD tests.
struct test_params {
  subcarrier_spacing                     scs;
  uint8_t                                min_k2;
  std::optional<tdd_ul_dl_config_common> tdd_config;
};

std::ostream& operator<<(std::ostream& out, const test_params& params)
{
  out << fmt::format("scs={}kHz, min_k2={}", scs_to_khz(params.scs), params.min_k2);
  return out;
}

/// Common tester class used by FDD and TDD unit tests for the RA scheduler.
class base_ra_scheduler_test : public sub_scheduler_test_environment
{
protected:
  base_ra_scheduler_test(duplex_mode dplx_mode, const test_params& params_) :
    sub_scheduler_test_environment(get_sched_req(dplx_mode, params_),
                                   std::make_unique<dummy_pdcch_resource_allocator>()),
    params(params_),
    dummy_pdcch_sch(static_cast<dummy_pdcch_resource_allocator&>(*this->pdcch_alloc))
  {
    // Run slot once so that the resource grid gets initialized with the initial slot.
    this->run_slot();
  }

  ~base_ra_scheduler_test() override { this->flush_events(); }

  void do_run_slot() final
  {
    ra_sch.run_slot(res_grid);
    test_result_consistency();
  }

  static sched_cell_configuration_request_message get_sched_req(duplex_mode dplx_mode, const test_params& t_params)
  {
    cell_config_builder_params builder_params =
        create(dplx_mode,
               frequency_range::FR1,
               t_params.scs == subcarrier_spacing::kHz30 ? bs_channel_bandwidth::MHz20 : bs_channel_bandwidth::MHz10);
    builder_params.scs_common = t_params.scs;
    builder_params.min_k2     = t_params.min_k2;
    builder_params.min_k1     = builder_params.min_k2;
    if (dplx_mode == duplex_mode::TDD) {
      builder_params.tdd_ul_dl_cfg_common = t_params.tdd_config;
    }

    return sched_config_helper::make_default_sched_cell_configuration_request(builder_params);
  }

  /// \brief consistency checks that can be common to all test cases.
  void test_result_consistency()
  {
    // Check all RARs have an associated PDCCH (the sched test suite already checks if all PDCCHs have an associated
    // PDSCH, but not the other way around).
    for (const rar_information& rar : scheduled_rars(0)) {
      ASSERT_TRUE(std::any_of(scheduled_dl_pdcchs().begin(), scheduled_dl_pdcchs().end(), [&rar](const auto& pdcch) {
        return pdcch.ctx.rnti == rar.pdsch_cfg.rnti;
      }));
    }

    // Msg3 grant checks.
    unsigned nof_grants = 0;
    unsigned nof_puschs = 0;
    for (const rar_information& rar : scheduled_rars(0)) {
      nof_grants += rar.grants.size();
    }
    for (unsigned i = 0; i != cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list.size();
         ++i) {
      unsigned nof_msg3_matches = 0;
      ASSERT_TRUE(msg3_consistent_with_rars(scheduled_rars(0), scheduled_msg3_newtxs(i), nof_msg3_matches))
          << "Msg3 PUSCHs parameters must match the content of the RAR grants";
      nof_puschs += nof_msg3_matches;
    }
    ASSERT_EQ(nof_grants, nof_puschs);

    ASSERT_TRUE(res_grid[0].result.dl.ue_grants.empty());
    ASSERT_TRUE(res_grid[0].result.dl.bc.sibs.empty());
  }

  template <typename Func = noop_operation>
  void run_slot_until_next_rach_opportunity(const Func& func = {})
  {
    while (not cell_cfg.is_fully_ul_enabled(this->next_slot_rx() - 1)) {
      run_slot();
      func();
    }
  }

  void handle_rach_indication(const rach_indication_message& rach) { ra_sch.handle_rach_indication(rach); }

  static rach_indication_message::preamble create_preamble()
  {
    static auto next_rnti = test_rng::uniform_int<unsigned>(to_value(rnti_t::MIN_CRNTI), to_value(rnti_t::MAX_CRNTI));
    static const auto rnti_inc = test_rng::uniform_int<unsigned>(1, 5);

    rach_indication_message::preamble preamble =
        test_helper::create_preamble(test_rng::uniform_int<unsigned>(0, 63), to_rnti(next_rnti));
    preamble.time_advance =
        phy_time_unit::from_seconds(std::uniform_real_distribution<double>{0, 2005e-6}(test_rng::tls_gen()));
    next_rnti += rnti_inc;
    return preamble;
  }

  rach_indication_message create_rach_indication(unsigned nof_preambles)
  {
    std::vector<rach_indication_message::preamble> preambles;
    for (unsigned i = 0; i != nof_preambles; ++i) {
      preambles.push_back(create_preamble());
    }
    return test_helper::create_rach_indication(next_slot_rx() - 1, preambles);
  }

  ul_crc_indication create_crc_indication(span<const ul_sched_info> puschs, bool ack)
  {
    return test_helper::create_crc_indication(next_slot_rx() - 1, puschs, ack);
  }

  const pusch_time_domain_resource_allocation& get_pusch_td_resource(uint8_t time_resource) const
  {
    return cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list[time_resource];
  }

  span<const pdcch_dl_information> scheduled_dl_pdcchs() const { return res_grid[0].result.dl.dl_pdcchs; }

  span<const rar_information> scheduled_rars(uint8_t time_resource) const
  {
    unsigned k0 = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list[time_resource].k0;
    return res_grid[k0].result.dl.rar_grants;
  }

  span<const ul_sched_info> scheduled_msg3_newtxs(uint8_t time_resource) const
  {
    return res_grid[ra_helper::get_msg3_delay(cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.scs,
                                              get_pusch_td_resource(time_resource).k2)]
        .result.ul.puschs;
  }

  bool rar_ul_grant_consistent_with_rach_preamble(const rar_ul_grant&                      rar_grant,
                                                  const rach_indication_message::preamble& preamb) const
  {
    return rar_grant.temp_crnti == preamb.tc_rnti and rar_grant.rapid == preamb.preamble_id and
           rar_grant.ta == preamb.time_advance.to_Ta(cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.scs);
  }

  /// \brief Checks whether the parameters of the scheduled RAR grants, namely "rapid" and "TA", match those found
  /// in the detected preambles that compose the provided RACH indication.
  ///
  /// \param rars scheduled RARs.
  /// \param rach_ind RACH indication for a given slot and cell, that may contain multiple detected preambles.
  /// \param nof_detected number of RAR grants of the rand_ind that were detected in the scheduled RARs.
  /// \return true if RAR grants parameters match those of the detected preambles. False otherwise.
  bool rars_consistent_with_rach_indication(span<const rar_information>    rars,
                                            const rach_indication_message& rach_ind,
                                            unsigned&                      nof_detected)
  {
    nof_detected = 0;
    for (const rach_indication_message::occasion& occ : rach_ind.occasions) {
      // As per Section 5.1.3, TS 38.321, and from Section 5.3.2, TS 38.211, slot_idx uses as the numerology of
      // reference 15kHz for long PRACH Formats (i.e, slot_idx = subframe index); whereas, for short PRACH formats, it
      // uses the same numerology as the SCS common (i.e, slot_idx = actual slot index within the frame).
      const unsigned slot_idx =
          is_long_preamble(
              prach_configuration_get(
                  band_helper::get_freq_range(cell_cfg.band()),
                  band_helper::get_duplex_mode(cell_cfg.band()),
                  cell_cfg.params.ul_cfg_common.init_ul_bwp.rach_cfg_common->rach_cfg_generic.prach_config_index)
                  .format)
              ? rach_ind.slot_rx.subframe_index()
              : rach_ind.slot_rx.slot_index();
      rnti_t                 ra_rnti = ra_helper::get_ra_rnti(slot_idx, occ.start_symbol, occ.frequency_index);
      const rar_information* rar     = find_rar(rars, ra_rnti);
      if (rar == nullptr) {
        continue;
      }

      TESTASSERT(occ.preambles.size() >= rar->grants.size(),
                 "Cannot allocate more RARs than the number of detected preambles");

      for (const rar_ul_grant& grant : rar->grants) {
        const auto* it =
            std::find_if(occ.preambles.begin(), occ.preambles.end(), [crnti = grant.temp_crnti](const auto& preamble) {
              return preamble.tc_rnti == crnti;
            });
        TESTASSERT(it != occ.preambles.end(), "RAR grant with no associated preamble was allocated");
        const rach_indication_message::preamble& preamble = *it;

        if (not rar_ul_grant_consistent_with_rach_preamble(grant, preamble)) {
          return false;
        }
        nof_detected++;
      }
    }
    return true;
  }

  /// \brief For a slot to be valid for RAR in TDD mode, the RAR PDCCH, RAR PDSCH and Msg3 PUSCH must fall in DL, DL
  /// and UL slots, respectively.
  bool is_slot_valid_for_rar_pdcch(unsigned delay = 0) const
  {
    if (not cell_cfg.is_tdd()) {
      // FDD case.
      return true;
    }
    slot_point pdcch_slot = res_grid[delay].slot;

    if (not cell_cfg.is_dl_enabled(pdcch_slot)) {
      // slot for PDCCH is not DL slot.
      return false;
    }
    const auto& pdsch_list = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
    if (std::none_of(pdsch_list.begin(), pdsch_list.end(), [this, &pdcch_slot](const auto& pdsch) {
          return cell_cfg.is_dl_enabled(pdcch_slot + pdsch.k0) and
                 not csi_helper::is_csi_rs_slot(cell_cfg, pdcch_slot + pdsch.k0);
        })) {
      // slot for PDSCH is not DL slot.
      return false;
    }
    const auto& pusch_list = cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;
    return not std::none_of(pusch_list.begin(), pusch_list.end(), [this, &pdcch_slot](const auto& pusch) {
      return cell_cfg.is_fully_ul_enabled(
          pdcch_slot +
          ra_helper::get_msg3_delay(cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.scs, pusch.k2));
    });
  }

  bool is_slot_valid_for_msg3_retx_pdcch() const
  {
    if (not cell_cfg.is_tdd()) {
      // FDD case.
      return true;
    }
    slot_point pdcch_slot = res_grid[0].slot;

    if (not cell_cfg.is_dl_enabled(pdcch_slot)) {
      // slot for PDCCH is not DL slot.
      return false;
    }

    const auto& pusch_list = cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;
    return not std::none_of(pusch_list.begin(), pusch_list.end(), [this, &pdcch_slot](const auto& pusch) {
      return cell_cfg.is_fully_ul_enabled(pdcch_slot + pusch.k2);
    });
  }

  bool msg3_consistent_with_rars(span<const rar_information> rars,
                                 span<const ul_sched_info>   ul_grants,
                                 unsigned&                   nof_matches)
  {
    nof_matches = 0;
    for (const rar_information& rar : rars) {
      for (const rar_ul_grant& grant : rar.grants) {
        const auto* it = std::find_if(ul_grants.begin(), ul_grants.end(), [&grant](const auto& ul_info) {
          return ul_info.pusch_cfg.rnti == grant.temp_crnti;
        });
        if (it != ul_grants.end()) {
          const pusch_information& pusch           = it->pusch_cfg;
          uint8_t                  freq_assignment = ra_frequency_type1_get_riv(
              ra_frequency_type1_configuration{cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.crbs.length(),
                                               pusch.rbs.type1().start(),
                                               pusch.rbs.type1().length()});
          bool grant_matches = pusch.symbols == get_pusch_td_resource(grant.time_resource_assignment).symbols and
                               freq_assignment == grant.freq_resource_assignment and
                               *pusch.bwp_cfg == cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params;
          if (not grant_matches) {
            return false;
          }
          nof_matches++;
        }
      }
    }
    return true;
  }

  slot_interval get_rar_window(slot_point rach_slot_rx) const
  {
    slot_point rar_win_start;
    for (unsigned i = 1; i != rach_slot_rx.nof_slots_per_frame(); ++i) {
      if (cell_cfg.is_dl_enabled(rach_slot_rx + i)) {
        rar_win_start = rach_slot_rx + i;
        break;
      }
    }
    return {rar_win_start,
            rar_win_start + cell_cfg.params.ul_cfg_common.init_ul_bwp.rach_cfg_common->rach_cfg_generic.ra_resp_window};
  }

  bool is_in_rar_window(slot_point rach_slot_rx) const
  {
    slot_interval rar_win = get_rar_window(rach_slot_rx);
    return rar_win.contains(result_slot_tx());
  }

  static const rar_information* find_rar(span<const rar_information> rars, rnti_t ra_rnti)
  {
    const auto* rar_it =
        std::find_if(rars.begin(), rars.end(), [ra_rnti](const auto& rar) { return rar.pdsch_cfg.rnti == ra_rnti; });
    return rar_it != rars.end() ? &*rar_it : nullptr;
  }

  slot_point result_slot_tx() const { return res_grid[0].slot; }

  test_params params;

  dummy_pdcch_resource_allocator& dummy_pdcch_sch;
  ra_scheduler                    ra_sch{cell_cfg, dummy_pdcch_sch, ev_logger, metrics_hdlr};
};

class ra_scheduler_tdd_test : public base_ra_scheduler_test, public ::testing::TestWithParam<test_params>
{
protected:
  ra_scheduler_tdd_test() : base_ra_scheduler_test(duplex_mode::TDD, GetParam()) {}
};

TEST_P(ra_scheduler_tdd_test, when_no_rbs_are_available_then_rar_is_scheduled_in_following_slot)
{
  // Forward RACH indication to scheduler.
  run_slot_until_next_rach_opportunity();
  rach_indication_message rach_ind = create_rach_indication(1);
  handle_rach_indication(rach_ind);
  slot_interval rar_win = get_rar_window(rach_ind.slot_rx);

  // Forbid PDCCH alloc in the next slot.
  this->dummy_pdcch_sch.fail_pdcch_alloc_cond = [next_sl = res_grid[1].slot](slot_point pdcch_slot) {
    return pdcch_slot == next_sl;
  };

  // Process slot and schedule RAR in a future slot.
  run_slot();

  // Given that the resource grid was already filled for this slot, no RAR should be scheduled.
  ASSERT_TRUE(res_grid[0].result.dl.dl_pdcchs.empty());

  // Note: There is a small chance that the RAR was not scheduled because the next "n" (< max RA DL sched delay) slots
  // were not valid for DL RAR scheduling (not DL slots or they contain CSI-RS). For this reason, we may need to
  // call "run_slot" again.
  const unsigned attempts_remaining = 2;
  int            td_res             = -1;
  for (unsigned a = 0; a != attempts_remaining and td_res == -1; ++a) {
    unsigned       n     = 1;
    const unsigned max_n = 6;
    for (; rar_win.contains(res_grid[0].slot + n) and n != max_n; ++n) {
      if (not is_slot_valid_for_rar_pdcch(n)) {
        ASSERT_TRUE(scheduled_dl_pdcchs().empty())
            << fmt::format("RAR PDCCH allocated in invalid slot {}", result_slot_tx());
        continue;
      }

      // RAR PDCCH scheduled.
      ASSERT_EQ(res_grid[n].result.dl.dl_pdcchs.size(), 1) << fmt::format("No RAR PDCCH in slot {}", res_grid[n].slot);
      td_res = res_grid[n].result.dl.dl_pdcchs[0].dci.ra_f1_0.time_resource;
      break;
    }
    for (unsigned i = 0; i != n; ++i) {
      // Update current slot to the slot when PDCCH was scheduled.
      run_slot();
    }
  }

  ASSERT_GE(td_res, 0) << "RAR PDCCH not found";
  // RAR PDSCH allocated.
  span<const rar_information> rars = scheduled_rars(td_res);
  ASSERT_EQ(rars.size(), 1);
  unsigned nof_grants = 0;
  ASSERT_TRUE(rars_consistent_with_rach_indication(rars, rach_ind, nof_grants));
  ASSERT_EQ(nof_grants, 1);
  ASSERT_EQ(nof_grants, rars[0].grants.size()) << "All scheduled RAR grants must be for the provided occasion";
  // Msg3 scheduled.
  ASSERT_EQ(scheduled_msg3_newtxs(rars[0].grants[0].time_resource_assignment).size(), nof_grants)
      << "Number of Msg3 PUSCHs must match number of RARs";
}

INSTANTIATE_TEST_SUITE_P(ra_scheduler,
                         ra_scheduler_tdd_test,
                         ::testing::Values(test_params{.scs = subcarrier_spacing::kHz15, .min_k2 = 2},
                                           test_params{.scs = subcarrier_spacing::kHz30, .min_k2 = 2}));

class failed_rar_scheduler_test : public base_ra_scheduler_test, public ::testing::Test
{
protected:
  failed_rar_scheduler_test() :
    base_ra_scheduler_test(duplex_mode::TDD, test_params{.scs = subcarrier_spacing::kHz30, .min_k2 = 2})
  {
  }
};

TEST_F(failed_rar_scheduler_test, when_rar_grants_not_scheduled_within_window_then_future_rars_can_still_be_scheduled)
{
  unsigned grants_to_fail = MAX_NOF_DU_UES;
  for (unsigned i = 0; i * MAX_PREAMBLES_PER_PRACH_OCCASION < grants_to_fail; ++i) {
    // Keep pushing RACH indications which will lead to failed to be scheduled RARs.
    run_slot_until_next_rach_opportunity();
    rach_indication_message rach_ind = create_rach_indication(MAX_PREAMBLES_PER_PRACH_OCCASION);
    handle_rach_indication(rach_ind);
    slot_interval rar_win = get_rar_window(rach_ind.slot_rx);

    // Forbid PDCCH alloc within the RAR window to force failure.
    this->dummy_pdcch_sch.fail_pdcch_alloc_cond = [](slot_point pdcch_slot) { return true; };
    while (this->res_grid[1].slot < rar_win.stop()) {
      run_slot();

      ASSERT_TRUE(res_grid[0].result.ul.puschs.empty());
      ASSERT_TRUE(res_grid[0].result.dl.rar_grants.empty());
      ASSERT_TRUE(res_grid[0].result.dl.dl_pdcchs.empty());
    }
  }

  // Now, we should be able to schedule RARs again.
  this->dummy_pdcch_sch.fail_pdcch_alloc_cond = [](slot_point) { return false; };
  auto ack_msg3s                              = [this]() {
    if (not this->res_grid[0].result.ul.puschs.empty()) {
      // Forward CRC=OK for Msg3s.
      ul_crc_indication crc_ind = create_crc_indication(res_grid[0].result.ul.puschs, true);
      this->ra_sch.handle_crc_indication(crc_ind);
    }
  };
  unsigned grant_count = 0;
  while (grant_count < MAX_NOF_DU_UES) {
    run_slot_until_next_rach_opportunity(ack_msg3s);
    rach_indication_message rach_ind = create_rach_indication(test_rng::uniform_int<unsigned>(1, 10));
    handle_rach_indication(rach_ind);

    for (unsigned nof_sched_grants = 0; nof_sched_grants < rach_ind.occasions[0].preambles.size();) {
      run_slot();
      ack_msg3s();

      if (not is_slot_valid_for_rar_pdcch()) {
        ASSERT_TRUE(scheduled_dl_pdcchs().empty())
            << fmt::format("RAR PDCCH allocated in invalid slot {}", result_slot_tx());
        continue;
      }
      if (not is_in_rar_window(rach_ind.slot_rx)) {
        ASSERT_TRUE(scheduled_dl_pdcchs().empty()) << fmt::format("RAR PDCCH allocated after RAR window");
        break;
      }

      slot_point last_alloc_slot = res_grid[0].slot;

      if (csi_helper::is_csi_rs_slot(cell_cfg, last_alloc_slot)) {
        // CSI-RS slot, skip it.
        continue;
      }
      // RAR PDSCH allocated.
      ASSERT_EQ(scheduled_rars(0).size(), 1);
      unsigned nof_grants = 0;
      ASSERT_TRUE(rars_consistent_with_rach_indication(scheduled_rars(0), rach_ind, nof_grants));
      ASSERT_EQ(nof_grants, scheduled_rars(0)[0].grants.size())
          << "All scheduled RAR grants must be for the provided occasion";

      nof_sched_grants += nof_grants;
    }

    grant_count += rach_ind.occasions[0].preambles.size();
  }
}

struct test_params2 {
  frequency_range                        fr;
  unsigned                               min_k;
  std::optional<tdd_ul_dl_config_common> tdd_cfg;
};

/// Test suite common to different FRs, duplex modes, k values.
class ra_scheduler_common_test : public ra_scheduler_setup, public ::testing::TestWithParam<test_params2>
{
public:
  ra_scheduler_common_test() : ra_scheduler_setup(get_sched_req(GetParam())) {}

  static sched_cell_configuration_request_message get_sched_req(const test_params2& t_params)
  {
    cell_config_builder_params builder_params =
        create(t_params.tdd_cfg.has_value() ? duplex_mode::TDD : duplex_mode::FDD, t_params.fr);
    builder_params.min_k2               = t_params.min_k;
    builder_params.min_k1               = t_params.min_k;
    builder_params.tdd_ul_dl_cfg_common = t_params.tdd_cfg;
    return sched_config_helper::make_default_sched_cell_configuration_request(builder_params);
  }

  /// Check if the grid contains any PDCCH, RAR PDSCH or PUSCH, indicating RA scheduler scheduled activity.
  bool no_grants_scheduled() const
  {
    for (unsigned i = 0; i != max_k_value; ++i) {
      const auto& result = res_grid[i].result;
      if (not(result.dl.dl_pdcchs.empty() and result.dl.rar_grants.empty() and result.dl.ul_pdcchs.empty() and
              result.ul.puschs.empty())) {
        return false;
      }
    }
    return true;
  }
};

/// This test verifies that the cell resource grid remains empty when no RACH indications arrive to the RA scheduler.
TEST_P(ra_scheduler_common_test, when_no_rach_indication_received_then_no_rar_allocated)
{
  for (unsigned i = 0, nof_slots = 10; i < nof_slots; ++i) {
    run_slot();
    ASSERT_TRUE(no_grants_scheduled());
  }
}

TEST_P(ra_scheduler_common_test,
       when_rach_indication_with_single_preamble_received_then_one_rar_and_one_msg3_are_allocated)
{
  // Forward single RACH occasion with multiple preambles.
  rach_indication_message one_rach = create_rach_indication(1);
  handle_rach_indication(one_rach);

  unsigned all_pdcch_count = 0;
  unsigned all_pusch_count = 0;
  for (unsigned slot_count = 0, max_slot_count = 1000; slot_count < max_slot_count and tracker.nof_msg3_acked() == 0;
       ++slot_count) {
    run_slot();

    ASSERT_EQ(this->res_grid[0].result.dl.ue_grants.size(), 0);
    ASSERT_EQ(this->res_grid[0].result.dl.ul_pdcchs.size(), 0);
    all_pdcch_count += this->res_grid[0].result.dl.dl_pdcchs.size();
    const auto& puschs = this->res_grid[0].result.ul.puschs;
    all_pusch_count += puschs.size();

    if (not puschs.empty()) {
      // Event: Positive CRC is sent.
      handle_crc_indication(test_helper::create_crc_indication(last_slot_tx(), puschs, true));
    }
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(10));
  ASSERT_EQ(all_pdcch_count, 1) << "Only one DL PDCCH (the RAR PDCCH) is expected";
  ASSERT_EQ(all_pusch_count, 1) << "Only one PUSCH (the Msg3) is expected";
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

    const auto& puschs = this->res_grid[0].result.ul.puschs;
    if (not puschs.empty()) {
      // Event: Positive CRC is sent for Msg3s.
      handle_crc_indication(test_helper::create_crc_indication(last_slot_tx(), puschs, true));
    }
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

    const auto& puschs = this->res_grid[0].result.ul.puschs;
    if (not puschs.empty()) {
      // Event: Positive CRC is sent for Msg3s.
      handle_crc_indication(test_helper::create_crc_indication(last_slot_tx(), puschs, true));
    }
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

    const auto& puschs = this->res_grid[0].result.ul.puschs;
    if (not puschs.empty()) {
      // Event: Negative CRC is sent for Msg3s.
      handle_crc_indication(test_helper::create_crc_indication(last_slot_tx(), puschs, false));
    }
  }

  // ACK following Msg3s.
  ASSERT_EQ(tracker.nof_msg3_acked(), 0);
  for (unsigned slot_count = 0, max_slots = 1000; slot_count != max_slots and tracker.nof_msg3_acked() < nof_preambles;
       ++slot_count) {
    run_slot();

    const auto& puschs = this->res_grid[0].result.ul.puschs;
    if (not puschs.empty()) {
      // Event: Positive CRC is sent for Msg3 retransmissions.
      handle_crc_indication(test_helper::create_crc_indication(last_slot_tx(), puschs, true));
    }
  }

  ASSERT_FALSE(grants_scheduled_in_next_slots(20));
  ASSERT_EQ(tracker.nof_msg3_acked(), nof_preambles);
  ASSERT_EQ(tracker.nof_msg3_newtxs(), nof_preambles);
  ASSERT_GE(tracker.nof_msg3_retxs(), nof_preambles);
}

INSTANTIATE_TEST_SUITE_P(
    ra_scheduler,
    ra_scheduler_common_test,
    ::testing::Values(
        // FR1, FDD.
        test_params2{frequency_range::FR1, 2},
        test_params2{frequency_range::FR1, 4},
        // FR1, TDD.
        test_params2{frequency_range::FR1, 2, create_tdd_pattern(tdd_pattern_profile_fr1_30khz::DDDDDDDSUU)},
        test_params2{frequency_range::FR1, 4, create_tdd_pattern(tdd_pattern_profile_fr1_30khz::DDDDDDDSUU)},
        test_params2{frequency_range::FR1, 2, create_tdd_pattern(tdd_pattern_profile_fr1_30khz::DDDSU)},
        test_params2{frequency_range::FR1, 1, create_tdd_pattern(tdd_pattern_profile_fr1_30khz::DSUU)},
        // FR2, TDD.
        test_params2{frequency_range::FR2, 1, create_tdd_pattern(tdd_pattern_profile_fr2_120khz::DDDSU)}));

} // namespace
