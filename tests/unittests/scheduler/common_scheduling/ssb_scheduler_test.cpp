/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "../test_utils/sched_custom_test_bench.h"
#include "lib/scheduler/cell/resource_grid.h"
#include "lib/scheduler/common_scheduling/ssb_scheduler.h"
#include "tests/test_doubles/scheduler/scheduler_config_helper.h"
#include "tests/unittests/scheduler/test_utils/config_generators.h"
#include "ocudu/ran/frame_types.h"
#include "ocudu/ran/ssb/ssb_mapping.h"
#include "ocudu/support/test_utils.h"
#include "fmt/ostream.h"
#include <gtest/gtest.h>

using namespace ocudu;

namespace {
struct ssb_params {
  ssb_periodicity    periodicity = ssb_periodicity::ms5;
  nr_band            band        = nr_band::n3;
  arfcn_t            freq_arfcn  = 536020;
  uint8_t            L_max       = 4U;
  subcarrier_spacing ssb_scs     = subcarrier_spacing::kHz15;
  // Parameters that are either taken as default or generated randomly.
  ssb_bitmap_t ssb_bitmap;
  unsigned     offset_to_point_A = 14;
  uint8_t      k_ssb             = 0;
};

std::ostream& operator<<(std::ostream& os, const ssb_params& params)
{
  os << "p_" << ssb_periodicity_to_value(params.periodicity);
  os << "_n" << fmt::underlying(params.band);
  os << "_arfnc_" << params.freq_arfcn.value();
  // Apply cast to avoid fmt:: complains.
  os << "_L_max_" << static_cast<unsigned>(params.L_max);
  os << "_scs_" << to_string(params.ssb_scs);
  if (params.L_max == 4) {
    os << "_ssb_bmap_" << fmt::format("{:#06b}", params.ssb_bitmap.to_uint64());
  } else {
    os << "_ssb_bmap_" << fmt::format("{:#010b}", params.ssb_bitmap.to_uint64());
  }
  os << "_off_pA_" << fmt::format("{}", params.offset_to_point_A);
  // Apply cast to avoid fmt:: complains.
  os << "_k_SSB_" << fmt::format("{}", static_cast<unsigned>(params.k_ssb));

  return os;
}

} // namespace

template <>
struct fmt::formatter<ssb_params> : ostream_formatter {};

static ssb_params gen_ssb_params_with_rnd_bitmap(const ssb_params& params)
{
  ssb_params derived_params = params;

  derived_params.ssb_bitmap =
      ssb_bitmap_t(test_rgen::uniform_int<uint8_t>(1, params.L_max * params.L_max - 1), params.L_max);

  return derived_params;
}

static ssb_params gen_random_ssb_freq_params(const ssb_params& params)
{
  auto get_max_offset_to_point_A = [](ssb_pattern_case ssb_case) {
    switch (ssb_case) {
      case ssb_pattern_case::A:
        // For case A (20MHz, SCS 15kHz), there are 106 within the Initial DL BWP.
        return 106U - NOF_SSB_PRBS - 1;
      case ssb_pattern_case::B:
      case ssb_pattern_case::C:
        // For case B and C (20MHz, SCS 30kHz), there are 52 within the Initial DL BWP.
        return 52 - NOF_SSB_PRBS - 1;
      default:
        ocudu_assertion_failure("Only SSB cases A, B, C are supported in this test");
        return 0U;
    }
  };

  auto get_max_k_SSB = [](ssb_pattern_case ssb_case) {
    switch (ssb_case) {
      case ssb_pattern_case::A:
        return 11U;
      case ssb_pattern_case::B:
      case ssb_pattern_case::C:
        return 23U;
      default:
        ocudu_assertion_failure("Only SSB cases A, B, C are supported in this test");
        return 0U;
    }
  };

  const ssb_pattern_case ssb_case = band_helper::get_ssb_pattern(params.band, params.ssb_scs);

  ssb_params derived_params = params;

  derived_params.offset_to_point_A = test_rgen::uniform_int<unsigned>(0, get_max_offset_to_point_A(ssb_case) - 1);
  if (ssb_case == ssb_pattern_case::B or ssb_case == ssb_pattern_case::C) {
    // With case B and C, offset_to_point_A must be an even number.
    derived_params.offset_to_point_A = (derived_params.offset_to_point_A / 2) * 2;
  }

  derived_params.k_ssb = test_rgen::uniform_int<unsigned>(0, get_max_k_SSB(ssb_case) - 1);

  derived_params.ssb_bitmap =
      ssb_bitmap_t(test_rgen::uniform_int<uint8_t>(1, params.L_max * params.L_max - 1), params.L_max);

  return derived_params;
}

static sched_cell_configuration_request_message make_cell_cfg_req_msg(const ssb_params& params)
{
  sched_cell_configuration_request_message msg = sched_config_helper::make_default_sched_cell_configuration_request();
  msg.ran.dl_carrier.arfcn_f_ref               = params.freq_arfcn;
  msg.ran.dl_carrier.band                      = params.band;
  msg.ran.dl_cfg_common.freq_info_dl.offset_to_point_a = params.offset_to_point_A;
  msg.ran.dl_cfg_common.init_dl_bwp.generic_params.scs = params.ssb_scs;
  msg.ran.ssb_cfg.scs                                  = params.ssb_scs;

  // Generate a random
  msg.ran.ssb_cfg.ssb_bitmap        = ssb_bitmap_t(test_rgen::uniform_int<uint8_t>(1, params.L_max - 1), params.L_max);
  msg.ran.ssb_cfg.ssb_period        = params.periodicity;
  msg.ran.ssb_cfg.offset_to_point_A = params.offset_to_point_A;
  msg.ran.ssb_cfg.k_ssb             = params.k_ssb;
  // Change Carrier parameters when SCS is 15kHz.
  if (params.ssb_scs == subcarrier_spacing::kHz15) {
    msg.ran.dl_cfg_common.freq_info_dl.scs_carrier_list.front().carrier_bandwidth = 106;
    msg.ran.dl_cfg_common.init_dl_bwp.generic_params.crbs =
        crb_interval{0, msg.ran.dl_cfg_common.freq_info_dl.scs_carrier_list.front().carrier_bandwidth};
  }
  // Change Carrier parameters when SCS is 30kHz.
  else if (params.ssb_scs == subcarrier_spacing::kHz30) {
    msg.ran.dl_cfg_common.freq_info_dl.scs_carrier_list.emplace_back(
        scs_specific_carrier{0, subcarrier_spacing::kHz30, 52});
    msg.ran.dl_cfg_common.init_dl_bwp.generic_params.crbs = {
        0, msg.ran.dl_cfg_common.freq_info_dl.scs_carrier_list[1].carrier_bandwidth};
  }
  msg.ran.dl_carrier.carrier_bw = bs_channel_bandwidth::MHz20;
  msg.ran.dl_carrier.nof_ant    = 1;

  return msg;
}

class ssb_sched_test_bench : public sched_basic_custom_test_bench
{
protected:
  ssb_sched_test_bench(const sched_cell_configuration_request_message& sched_cell_cfg_req) :
    sched_basic_custom_test_bench(config_helpers::make_default_scheduler_expert_config(), sched_cell_cfg_req),
    cutoff_freq(compute_cutoff_freq(cell_cfg.ssb_case, cell_cfg.dl_carrier.band)),
    sl_idx_with_ssb_case_A_B_C{0, 1, 2, 3},
    ssb_sched(cell_cfg)
  {
    ocudulog::init();
  }

  static arfcn_t compute_cutoff_freq(ssb_pattern_case ssb_case, nr_band band)
  {
    ocudu_assert(ssb_case < ssb_pattern_case::D, "Only case A, B and C supported in this test");
    if (ssb_case == ssb_pattern_case::C and not band_helper::is_paired_spectrum(band)) {
      return CUTOFF_FREQ_ARFCN_CASE_C_UNPAIRED;
    }
    return CUTOFF_FREQ_ARFCN_CASE_A_B_C;
  }

  void run_slot() { ssb_sched.run_slot(res_grid, current_sl_tx); }

  void slot_indication() { sched_basic_custom_test_bench::slot_indication(++current_sl_tx); }

  // Returns the possible starting OFDM symbols for SSB.
  std::array<uint8_t, 2> get_starting_ofdm_symbols(slot_point sl) const
  {
    if (cell_cfg.ssb_case == ssb_pattern_case::A or cell_cfg.ssb_case == ssb_pattern_case::C) {
      return {2, 8};
    }
    return sl.count() % 2U == 0 ? std::array<uint8_t, 2>{4, 8} : std::array<uint8_t, 2>{2, 6};
  }

  crb_interval get_ssb_crbs() const
  {
    const unsigned ssb_crb_start = cell_cfg.dl_cfg_common.init_dl_bwp.generic_params.scs == subcarrier_spacing::kHz15
                                       ? cell_cfg.ssb_cfg.offset_to_point_A.value()
                                       : cell_cfg.ssb_cfg.offset_to_point_A.value() / 2;
    const unsigned ssb_crb_stop =
        cell_cfg.ssb_cfg.k_ssb.value() > 0 ? ssb_crb_start + NOF_SSB_PRBS + 1 : ssb_crb_start + NOF_SSB_PRBS;

    return crb_interval(ssb_crb_start, ssb_crb_stop);
  }

  // Helper function to test the symbols and CRBs in the SSB information struct.
  void test_ssb_information(uint8_t expected_sym, const ssb_information& ssb_info) const
  {
    // Check OFDM symbols and frequency allocation in the ssb_information struct.
    // con
    const crb_interval expected_ssb_crbs = get_ssb_crbs();
    ASSERT_EQ(expected_ssb_crbs, ssb_info.crbs);
    ASSERT_EQ(ofdm_symbol_range(expected_sym, expected_sym + NOF_SYMB_PER_SSB), ssb_info.symbols);
  }

  void test_ssb_grid_allocation(const cell_slot_resource_grid& slot_res_grid, const ssb_information& ssb_info) const
  {
    // Verify resources on the left-side of SSB (lower CRBs) are unused.
    grant_info empty_space{cell_cfg.ssb_cfg.scs, {0, NOF_OFDM_SYM_PER_SLOT_NORMAL_CP}, {0, ssb_info.crbs.start()}};
    ASSERT_FALSE(slot_res_grid.collides(empty_space))
        << fmt::format("PRBs {} over symbols {} should be empty", empty_space.crbs, empty_space.symbols);

    // Verify resources on the left-side of SSB (lower CRBs) are unused.
    ASSERT_TRUE(cell_cfg.dl_cfg_common.init_dl_bwp.generic_params.crbs.contains(ssb_info.crbs))
        << "SSB falls outside the BWP";

    // Verify resources on the right-side of SSB (lower CRBs) are unused.
    ASSERT_FALSE(slot_res_grid.collides(empty_space))
        << fmt::format("PRBs {} over symbols {} should be empty", empty_space.crbs, empty_space.symbols);

    grant_info ssb_resources{cell_cfg.ssb_cfg.scs, ssb_info.symbols, ssb_info.crbs};
    ASSERT_TRUE(slot_res_grid.all_set(ssb_resources))
        << fmt::format("PRBs {} over symbols {} should be set", ssb_resources.crbs, ssb_resources.symbols);
  }

  const arfcn_t cutoff_freq;
  // Indices of slots (within the half-system frame) which can contains SSBs.
  std::array<unsigned, 4>   sl_idx_with_ssb_case_A_B_C;
  static constexpr unsigned NOF_SYMB_PER_SSB = 4U;

private:
  ssb_scheduler ssb_sched;
};

class ssb_sched_tester : public ssb_sched_test_bench, public ::testing::TestWithParam<ssb_params>
{
protected:
  ssb_sched_tester() : ssb_sched_test_bench(make_cell_cfg_req_msg(GetParam())) {}
};

TEST_P(ssb_sched_tester, test_time_dom_allocation)
{
  test_logger.info("{}", GetParam());

  // Get the SSB period in half radio frames
  const unsigned ssb_period_half_sfn = ssb_periodicity_to_value(cell_cfg.ssb_cfg.ssb_period) * 2U / 10U;

  for (; current_sl_tx.count() != 1000U; slot_indication()) {
    run_slot();

    if (const bool is_ssb_half_sfn = current_sl_tx.half_sfn() % ssb_period_half_sfn == 0U; is_ssb_half_sfn) {
      // Check whether the L_max matches the ssb_bitmap length.
      const uint8_t expected_l_max = cell_cfg.dl_carrier.arfcn_f_ref <= cutoff_freq ? 4U : 8U;
      ASSERT_EQ(cell_cfg.ssb_cfg.ssb_bitmap.get_L_max(), expected_l_max);

      const bool is_ssb_slot = cell_cfg.dl_carrier.arfcn_f_ref <= cutoff_freq ? current_sl_tx.hrf_slot_index() <= 1U
                                                                              : current_sl_tx.hrf_slot_index() <= 3U;
      if (is_ssb_slot) {
        constexpr unsigned NOF_SSB_BEAMS_PER_SLOT_FR1 = 2U;
        const auto         slot_bitmap                = cell_cfg.ssb_cfg.ssb_bitmap.slice<NOF_SSB_BEAMS_PER_SLOT_FR1>(
            current_sl_tx.hrf_slot_index() * NOF_SSB_BEAMS_PER_SLOT_FR1,
            current_sl_tx.hrf_slot_index() * NOF_SSB_BEAMS_PER_SLOT_FR1 + NOF_SSB_BEAMS_PER_SLOT_FR1);

        ASSERT_EQ(res_grid[0].result.dl.bc.ssb_info.size(), slot_bitmap.count())
            << fmt::format("Number of SSB PDUs not as expected at slot={}", current_sl_tx);

        const auto starting_symbs = get_starting_ofdm_symbols(current_sl_tx);
        auto       ssb_pdu_it     = res_grid[0].result.dl.bc.ssb_info.begin();
        for (unsigned i = 0, sz = slot_bitmap.size(); i != sz; ++i) {
          if (not slot_bitmap.test(i)) {
            continue;
          }
          // Test the correctness of SSB PDU fields.
          const ssb_information ssb_info = *ssb_pdu_it;
          ASSERT_EQ(ssb_info.ssb_index, current_sl_tx.hrf_slot_index() * NOF_SSB_BEAMS_PER_SLOT_FR1 + i);
          const crb_interval expected_ssb_crbs = get_ssb_crbs();
          ASSERT_EQ(expected_ssb_crbs, ssb_info.crbs);
          ASSERT_EQ(ofdm_symbol_range(starting_symbs[i], starting_symbs[i] + NOF_SYMB_PER_SSB), ssb_info.symbols);
          // Test whether the DL grid allocation matches the information in the SSB PDU.
          test_ssb_grid_allocation(res_grid[0].dl_res_grid, ssb_info);
          ++ssb_pdu_it;
        }
      } else {
        ASSERT_TRUE(res_grid[0].result.dl.bc.ssb_info.empty())
            << fmt::format("At slot={} the SSB info list is expected to be empty", current_sl_tx);

        grant_info empty_space{cell_cfg.dl_cfg_common.init_dl_bwp.generic_params.scs,
                               {0, NOF_OFDM_SYM_PER_SLOT_NORMAL_CP},
                               {0, cell_cfg.dl_cfg_common.freq_info_dl.scs_carrier_list[0].carrier_bandwidth}};
        ASSERT_FALSE(res_grid[0].dl_res_grid.collides(empty_space))
            << fmt::format("PRBs {} should be empty", empty_space.crbs);
      }
    }
  }
}

// In this macro, we pass some basic parameters and we use gen_ssb_params_with_rnd_bitmap() to generate a full set of
// test parameters with a random SSB bitmap.
INSTANTIATE_TEST_SUITE_P(
    test_ssb_allocation_time_domain_allocation,
    ssb_sched_tester,
    ::testing::Values(
        // clang-format off
    // FDD.
    // Test case A.
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms5, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms10, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms20, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms40, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms80, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    // Test case B. NOTE: the full scheduler doesn't support these configs yet, as it requires 15kHz for SRS common and 30kHz SSB SCS.
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms5, .band = nr_band::n5, .freq_arfcn = 176000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n5, .freq_arfcn = 176000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms20, .band = nr_band::n5, .freq_arfcn = 176000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms40, .band = nr_band::n5, .freq_arfcn = 176000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms80, .band = nr_band::n5, .freq_arfcn = 176000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    // TDD.
    // Test case A - freq. < cutoff_freq.
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms5, .band = nr_band::n41, .freq_arfcn = 512001, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n41, .freq_arfcn = 512001, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms20, .band = nr_band::n41, .freq_arfcn = 512001, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms40, .band = nr_band::n41, .freq_arfcn = 512001, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms80, .band = nr_band::n41, .freq_arfcn = 512001, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    // Test case C.
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms5, .band = nr_band::n50, .freq_arfcn = 292000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n50, .freq_arfcn = 292000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms20, .band = nr_band::n50, .freq_arfcn = 292000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms40, .band = nr_band::n50, .freq_arfcn = 292000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms80, .band = nr_band::n50, .freq_arfcn = 292000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    // Test case C - freq. > cutoff_freq.
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms5, .band = nr_band::n41, .freq_arfcn = 512004, .L_max = 8, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n41, .freq_arfcn = 512004, .L_max = 8, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms20, .band = nr_band::n41, .freq_arfcn = 512004, .L_max = 8, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms40, .band = nr_band::n41, .freq_arfcn = 512004, .L_max = 8, .ssb_scs = subcarrier_spacing::kHz30}),
    gen_ssb_params_with_rnd_bitmap(ssb_params{.periodicity = ssb_periodicity::ms80, .band = nr_band::n41, .freq_arfcn = 512004, .L_max = 8, .ssb_scs = subcarrier_spacing::kHz30})

        // clang-format on
        ),
    [](const testing::TestParamInfo<ssb_params>& params_item) { return fmt::format("{}", params_item.param); });

// In this macro, we pass some basic parameters and we use gen_random_ssb_freq_params() to generate a full set of
// test parameters with a random SSB bitmap, random offset_to_pointA and k_SSB.
INSTANTIATE_TEST_SUITE_P(test_freq_domain_allocation_for_ssb,
                         ssb_sched_tester,
                         ::testing::Values(
                             // clang-format off
    // Test case A.
    gen_random_ssb_freq_params(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n7, .freq_arfcn = 536020, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz15}),
    // Test case B.
    gen_random_ssb_freq_params(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n5, .freq_arfcn = 176000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    // Test case C, Unpaired spectrum, frequency <= 1.88GHz.
    gen_random_ssb_freq_params(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n50, .freq_arfcn = 292000, .L_max = 4, .ssb_scs = subcarrier_spacing::kHz30}),
    // Test case C, Unpaired spectrum, frequency > 1.88GHz.
    gen_random_ssb_freq_params(ssb_params{.periodicity = ssb_periodicity::ms10, .band = nr_band::n41, .freq_arfcn = 518000, .L_max = 8, .ssb_scs = subcarrier_spacing::kHz30})

                             // clang-format on
                             ),
                         [](const testing::TestParamInfo<ssb_params>& params_item) {
                           return fmt::format("{}", params_item.param);
                         });
