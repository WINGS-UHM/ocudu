/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/scheduler/ue_context/logical_channel_system.h"
#include "lib/scheduler/ue_context/ta_management_system.h"
#include "tests/unittests/scheduler/test_utils/config_generators.h"
#include "ocudu/support/test_utils.h"
#include <gtest/gtest.h>

using namespace ocudu;

TEST(inactive_ta_management_system_test, add_ue_returns_inactive_manager)
{
  scheduler_expert_config expert_cfg               = config_helpers::make_default_scheduler_expert_config();
  expert_cfg.ue.ta_control.ta_cmd_offset_threshold = -1; // Disable TA management.
  ta_management_system ta_sys(expert_cfg.ue.ta_control, subcarrier_spacing::kHz15);

  ue_logical_channel_repository ue_lc_chs;
  ue_ta_manager                 ta_mgr = ta_sys.add_ue(time_alignment_group::id_t{0}, ue_lc_chs);

  ASSERT_FALSE(ta_mgr.active()) << "TA manager should be inactive when TA management is disabled";
  auto ta_mgr_moved = std::move(ta_mgr);
  ASSERT_FALSE(ta_mgr_moved.active());
  ASSERT_FALSE(ta_mgr.active());
  ASSERT_EQ(ta_sys.nof_active_ues(), 0U);
}

class base_ta_management_system_test
{
protected:
  base_ta_management_system_test() :
    ul_scs(subcarrier_spacing::kHz30),
    ta_sys(expert_cfg.ue.ta_control, ul_scs),
    next_sl_tx(to_numerology_value(ul_scs), test_rgen::uniform_int<unsigned>(0, 10239))
  {
  }

  void run_slot()
  {
    next_sl_tx++;
    ta_sys.slot_indication(next_sl_tx);
  }

  /// Computes the N_TA update i.e. N_TA_new - N_TA_old value in T_C units, which will result in new TA command
  /// (new_ta_cmd) being computed by the TA manager.
  int64_t compute_n_ta_diff_leading_to_new_ta_cmd(uint8_t new_ta_cmd)
  {
    return static_cast<int64_t>(std::round(static_cast<double>(((static_cast<int64_t>(new_ta_cmd) - 31) * 16 * 64)) /
                                           static_cast<double>(pow2(to_numerology_value(ul_scs)))));
  }

  subcarrier_spacing          ul_scs;
  scheduler_expert_config     expert_cfg = config_helpers::make_default_scheduler_expert_config();
  logical_channel_config_pool cfg_pool;
  logical_channel_system      lc_ch_sys;

  ta_management_system ta_sys;

  slot_point next_sl_tx;
};

/// Test fixture for the TA management of a single UE.
class single_ue_ta_manager_test : public base_ta_management_system_test, public ::testing::Test
{
protected:
  single_ue_ta_manager_test() :
    ue_lc_chs(lc_ch_sys.create_ue(to_du_ue_index(0), ul_scs, false, cfg_pool.create({}))),
    ta_mgr(ta_sys.add_ue(time_alignment_group::id_t{0}, ue_lc_chs))
  {
    run_slot();
  }

  std::optional<dl_msg_lc_info> fetch_ta_cmd_mac_ce_allocation()
  {
    static const lcid_dl_sch_t lcid            = lcid_dl_sch_t::TA_CMD;
    static const unsigned      remaining_bytes = lcid.sizeof_ce() + FIXED_SIZED_MAC_CE_SUBHEADER_SIZE + 3;
    dl_msg_lc_info             subpdu{};
    if (ue_lc_chs.allocate_mac_ce(subpdu, remaining_bytes) > 0) {
      return subpdu;
    }
    return {};
  }

  std::optional<dl_msg_lc_info> run_until_next_ta_cmd(unsigned max_slots = 0)
  {
    max_slots = max_slots == 0 ? expert_cfg.ue.ta_control.measurement_period * 2 : max_slots;
    for (unsigned count = 0; count != max_slots; ++count) {
      run_slot();
      auto ta_cmd_mac_ce_alloc = fetch_ta_cmd_mac_ce_allocation();
      if (ta_cmd_mac_ce_alloc.has_value()) {
        return ta_cmd_mac_ce_alloc;
      }
    }
    return std::nullopt;
  }

  ue_logical_channel_repository ue_lc_chs;
  ue_ta_manager                 ta_mgr;
};

TEST_F(single_ue_ta_manager_test, ue_ta_manager_lifetime)
{
  ASSERT_EQ(ta_sys.nof_active_ues(), 1U);
  ASSERT_TRUE(ta_mgr.active());

  auto ta_mgr_moved = std::move(ta_mgr);
  ASSERT_EQ(ta_sys.nof_active_ues(), 1U);
  ASSERT_TRUE(ta_mgr_moved.active());
  ASSERT_FALSE(ta_mgr.active());

  ta_mgr = std::move(ta_mgr_moved);
  ASSERT_EQ(ta_sys.nof_active_ues(), 1U);
  ASSERT_TRUE(ta_mgr.active());
  ASSERT_FALSE(ta_mgr_moved.active());

  ta_mgr.reset();
  ASSERT_EQ(ta_sys.nof_active_ues(), 0U);
}

TEST_F(single_ue_ta_manager_test, ta_cmd_is_not_triggered_when_no_ul_n_ta_update_indication_are_reported)
{
  ASSERT_FALSE(run_until_next_ta_cmd().has_value()) << "TA command should not be triggered";
}

TEST_F(single_ue_ta_manager_test, ta_cmd_is_not_triggered_when_reported_ul_n_ta_update_indication_has_low_sinr)
{
  // Enqueue a UL N_TA update indication of low SINR.
  const uint8_t new_ta_cmd = 33;
  const float   ul_sinr    = expert_cfg.ue.ta_control.update_measurement_ul_sinr_threshold - 10;
  ta_mgr.handle_ul_n_ta_update_indication(
      time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd), ul_sinr);

  // Ensure MAC CE is not allocated for TA command.
  ASSERT_FALSE(run_until_next_ta_cmd().has_value()) << "TA command should not be triggered";
}

TEST_F(single_ue_ta_manager_test, ta_cmd_is_successfully_triggered)
{
  // Enqueue a UL N_TA update indication of high SINR.
  const uint8_t new_ta_cmd = 33;
  const float   ul_sinr    = expert_cfg.ue.ta_control.update_measurement_ul_sinr_threshold + 10;
  ta_mgr.handle_ul_n_ta_update_indication(
      time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd), ul_sinr);

  // Ensure MAC CE is allocated for TA command.
  std::optional<dl_msg_lc_info> ta_cmd_mac_ce_alloc = run_until_next_ta_cmd();
  ASSERT_TRUE(ta_cmd_mac_ce_alloc.has_value()) << "Missing TA command CE allocation";
  ASSERT_TRUE(ta_cmd_mac_ce_alloc->lcid == lcid_dl_sch_t::TA_CMD) << "TA command is not be triggered";
  ASSERT_TRUE(std::holds_alternative<ta_cmd_ce_payload>(ta_cmd_mac_ce_alloc->ce_payload))
      << "TA command CE payload is absent";
  auto ta_cmd_ce = std::get<ta_cmd_ce_payload>(ta_cmd_mac_ce_alloc->ce_payload);
  ASSERT_EQ(ta_cmd_ce.ta_cmd, new_ta_cmd) << "New TA command does not match the expected TA command value";
}

TEST_F(single_ue_ta_manager_test, verify_computed_new_ta_cmd_based_on_multiple_n_ta_diff_reported)
{
  // Expected value. Average of ta_values_reported excluding the outlier 45.
  const unsigned expected_new_ta_cmd = 34;
  const float    ul_sinr             = expert_cfg.ue.ta_control.update_measurement_ul_sinr_threshold + 10;

  // Pass enough TA values until outlier detection is activated.
  // Note: We use i%3 to add some variance in the samples.
  unsigned nof_init_samples = 10;
  for (unsigned i = 0; i < nof_init_samples; ++i) {
    ta_mgr.handle_ul_n_ta_update_indication(
        time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(33 + (i % 3)), ul_sinr);
  }
  ASSERT_TRUE(run_until_next_ta_cmd().has_value()) << "TA command should be triggered";
  ASSERT_FALSE(run_until_next_ta_cmd(expert_cfg.ue.ta_control.measurement_prohibit_period).has_value())
      << "TA command should not be triggered during prohibit period";

  // New measurement window.
  const std::vector<uint8_t> ta_values_reported = {35, 34, 45, 34, 33};
  for (const auto ta : ta_values_reported) {
    ta_mgr.handle_ul_n_ta_update_indication(
        time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(ta), ul_sinr);
  }

  std::optional<dl_msg_lc_info> ta_cmd_mac_ce_alloc =
      run_until_next_ta_cmd(expert_cfg.ue.ta_control.measurement_period * 4);
  ASSERT_TRUE(ta_cmd_mac_ce_alloc.has_value()) << "Missing TA command CE allocation";
  ASSERT_TRUE(ta_cmd_mac_ce_alloc->lcid == lcid_dl_sch_t::TA_CMD) << "TA command is not be triggered";
  ASSERT_TRUE(std::holds_alternative<ta_cmd_ce_payload>(ta_cmd_mac_ce_alloc->ce_payload))
      << "TA command CE payload is absent";
  auto ta_cmd_ce = std::get<ta_cmd_ce_payload>(ta_cmd_mac_ce_alloc->ce_payload);
  ASSERT_EQ(ta_cmd_ce.ta_cmd, expected_new_ta_cmd) << "New TA command does not match the expected TA command value";
}

TEST_F(single_ue_ta_manager_test, ta_cmd_is_not_triggered_if_ue_is_reset)
{
  // Enqueue a UL N_TA update indication of high SINR.
  const uint8_t new_ta_cmd = 33;
  const float   ul_sinr    = expert_cfg.ue.ta_control.update_measurement_ul_sinr_threshold + 10;
  ta_mgr.handle_ul_n_ta_update_indication(
      time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd), ul_sinr);

  // Reset the UE from the TA management system.
  ta_mgr.reset();

  // Ensure MAC CE is not allocated for TA command.
  ASSERT_FALSE(run_until_next_ta_cmd().has_value()) << "TA command should not be triggered after UE reset";
}

TEST_F(single_ue_ta_manager_test, n_ta_indications_ignored_during_prohibit_period)
{
  // Enqueue a UL N_TA update indication of high SINR.
  const uint8_t new_ta_cmd = 33;
  const float   ul_sinr    = expert_cfg.ue.ta_control.update_measurement_ul_sinr_threshold + 10;
  ta_mgr.handle_ul_n_ta_update_indication(
      time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd), ul_sinr);

  // Ensure MAC CE is allocated for TA command.
  ASSERT_TRUE(run_until_next_ta_cmd().has_value()) << "Missing TA command CE allocation";

  // Enqueue multiple UL N_TA update indications during prohibit period.
  const unsigned n_ta_indications_during_prohibit = expert_cfg.ue.ta_control.measurement_prohibit_period;
  for (unsigned i = 0; i < n_ta_indications_during_prohibit; ++i) {
    ta_mgr.handle_ul_n_ta_update_indication(
        time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd), ul_sinr);
    run_slot();
    ASSERT_FALSE(fetch_ta_cmd_mac_ce_allocation().has_value())
        << "TA command should not be triggered during prohibit period";
  }

  // Ensure MAC CE is not allocated for TA command.
  ASSERT_FALSE(run_until_next_ta_cmd().has_value()) << "TA command should not be triggered after UE reset";
}

TEST_F(single_ue_ta_manager_test, n_ta_indications_considered_after_prohibit_period)
{
  // Enqueue a UL N_TA update indication of high SINR.
  const uint8_t new_ta_cmd = 33;
  const float   ul_sinr    = expert_cfg.ue.ta_control.update_measurement_ul_sinr_threshold + 10;
  ta_mgr.handle_ul_n_ta_update_indication(
      time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd), ul_sinr);

  // Ensure MAC CE is allocated for TA command.
  std::optional<dl_msg_lc_info> ta_cmd_mac_ce_alloc = run_until_next_ta_cmd();
  ASSERT_TRUE(ta_cmd_mac_ce_alloc.has_value()) << "Missing TA command CE allocation";

  // Run through prohibit period.
  ta_cmd_mac_ce_alloc = run_until_next_ta_cmd(expert_cfg.ue.ta_control.measurement_prohibit_period);
  ASSERT_FALSE(ta_cmd_mac_ce_alloc.has_value()) << "TA command should not be triggered during prohibit period";

  // Enqueue a UL N_TA update indication after prohibit period.
  const uint8_t new_ta_cmd2 = 40;
  ta_mgr.handle_ul_n_ta_update_indication(
      time_alignment_group::id_t{0}, compute_n_ta_diff_leading_to_new_ta_cmd(new_ta_cmd2), ul_sinr);

  // Ensure MAC CE is allocated for TA command.
  ta_cmd_mac_ce_alloc = run_until_next_ta_cmd();
  ASSERT_TRUE(ta_cmd_mac_ce_alloc.has_value()) << "Missing TA command CE allocation";
  auto ta_cmd_ce = std::get<ta_cmd_ce_payload>(ta_cmd_mac_ce_alloc->ce_payload);
  ASSERT_EQ(ta_cmd_ce.ta_cmd, new_ta_cmd2) << "New TA command does not match the expected TA command value";
}
