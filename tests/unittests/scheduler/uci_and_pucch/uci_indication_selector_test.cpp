/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/scheduler/uci_scheduling/uci_indication_selector.h"
#include "ocudu/support/test_utils.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

using namespace ocudu;

namespace {

class test_timeout_notifier : public uci_indication_timeout_notifier
{
public:
  struct timeout_event {
    slot_point sl_rx;
    rnti_t     crnti;
    uci_action action;
  };

  void on_timeout(slot_point sl_rx, rnti_t crnti, const uci_action& action) override
  {
    events.push_back({sl_rx, crnti, action});
  }

  std::vector<timeout_event> events;
};

class uci_indication_selector_test : public ::testing::Test
{
protected:
  static constexpr unsigned timeout_slots = 16;
  static constexpr rnti_t   first_rnti    = to_rnti(0x4601);
  static constexpr rnti_t   second_rnti   = to_rnti(0x4602);

  static sched_result make_sched_result(std::initializer_list<pucch_info> pucchs)
  {
    sched_result result{};
    for (const auto& pucch : pucchs) {
      result.ul.pucchs.push_back(pucch);
    }
    return result;
  }

  static pucch_info make_pucch_grant(rnti_t crnti, pucch_format format, unsigned nof_harq_bits, bool sr = false)
  {
    pucch_info pucch{};
    pucch.crnti                      = crnti;
    pucch.bwp_cfg                    = nullptr;
    pucch.uci_bits.harq_ack_nof_bits = nof_harq_bits;
    pucch.uci_bits.sr_bits           = sr ? sr_nof_bits::one : sr_nof_bits::no_sr;
    pucch.set_format(format);
    return pucch;
  }

  static uci_indication::uci_pdu make_f0_or_f1_pdu(rnti_t                                            crnti,
                                                   std::initializer_list<mac_harq_ack_report_status> harq_bits,
                                                   bool                 sr_detected = false,
                                                   std::optional<float> snr_dB      = std::nullopt)
  {
    uci_indication::uci_pdu pdu{};
    pdu.ue_index = to_du_ue_index(0);
    pdu.crnti    = crnti;
    auto& f0f1   = pdu.pdu.emplace<uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu>();

    f0f1.sr_detected = sr_detected;
    f0f1.ul_sinr_dB  = snr_dB;
    f0f1.harqs.resize(harq_bits.size());
    std::copy(harq_bits.begin(), harq_bits.end(), f0f1.harqs.begin());

    return pdu;
  }

  test_timeout_notifier   timeout_notifier;
  uci_indication_selector selector{timeout_notifier, timeout_slots, MAX_PUCCH_PDUS_PER_SLOT};

  slot_point next_sl_tx{subcarrier_spacing::kHz30,
                        test_rgen::uniform_int<unsigned>(
                            0,
                            NOF_SFNS* NOF_SUBFRAMES_PER_FRAME* get_nof_slots_per_subframe(subcarrier_spacing::kHz30))};
};

TEST_F(uci_indication_selector_test, returns_selected_action_for_matching_pdu)
{
  selector.handle_result(next_sl_tx, make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2)}));

  auto action = selector.handle_uci_ind_pdu(
      next_sl_tx,
      make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack, mac_harq_ack_report_status::nack}, true, 10.0F));

  ASSERT_TRUE(action.has_value());
  EXPECT_TRUE(action->sr_detected);
  ASSERT_EQ(action->harq_ack_bits.size(), 2U);
  EXPECT_TRUE(action->harq_ack_bits.test(0));
  ASSERT_FALSE(action->harq_ack_bits.test(1));
  EXPECT_TRUE(timeout_notifier.events.empty());
}

TEST_F(uci_indication_selector_test, returns_nullopt_when_rnti_is_not_found)
{
  selector.handle_result(next_sl_tx, make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 1)}));

  auto action =
      selector.handle_uci_ind_pdu(next_sl_tx, make_f0_or_f1_pdu(second_rnti, {mac_harq_ack_report_status::ack}));

  ASSERT_FALSE(action.has_value());
  EXPECT_TRUE(timeout_notifier.events.empty());
}

TEST_F(uci_indication_selector_test, triggers_timeout_when_no_uci_indication_arrives)
{
  slot_point start_sl_tx = next_sl_tx;
  selector.handle_result(next_sl_tx++, make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2)}));

  for (slot_point last_slot = next_sl_tx + timeout_slots; next_sl_tx != last_slot; ++next_sl_tx) {
    ASSERT_TRUE(timeout_notifier.events.empty());
    selector.handle_result(next_sl_tx, make_sched_result({}));
  }

  ASSERT_EQ(timeout_notifier.events.size(), 1U);
  ASSERT_EQ(timeout_notifier.events.front().sl_rx, start_sl_tx);
  ASSERT_EQ(timeout_notifier.events.front().crnti, first_rnti);
  ASSERT_EQ(timeout_notifier.events.front().action.harq_ack_bits.size(), 2U);
  ASSERT_FALSE(timeout_notifier.events.front().action.harq_ack_bits.test(0));
  ASSERT_FALSE(timeout_notifier.events.front().action.harq_ack_bits.test(1));
}

TEST_F(uci_indication_selector_test, picks_higher_snr_for_multiple_expected_pucch_indications)
{
  selector.handle_result(next_sl_tx,
                         make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 1),
                                            make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 1, true)}));

  const bool  first_is_higher = test_rgen::uniform_int(0, 1) == 0;
  const float first_snr       = first_is_higher ? 10.0 : 1.0;
  const float second_snr      = first_is_higher ? 1.0 : 10.0;
  auto        first_action    = selector.handle_uci_ind_pdu(
      next_sl_tx, make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack}, false, first_snr));
  auto second_action = selector.handle_uci_ind_pdu(
      next_sl_tx, make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::nack}, true, second_snr));

  ASSERT_FALSE(first_action.has_value());
  ASSERT_TRUE(second_action.has_value());
  ASSERT_EQ(second_action->harq_ack_bits.size(), 1U);
  ASSERT_EQ(first_is_higher, second_action->harq_ack_bits.test(0));
  ASSERT_NE(first_is_higher, second_action->sr_detected);
}

TEST_F(uci_indication_selector_test, removes_single_entry_without_losing_other_slot_entries)
{
  selector.handle_result(next_sl_tx,
                         make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 1),
                                            make_pucch_grant(second_rnti, pucch_format::FORMAT_1, 1)}));

  auto second_rnti_action =
      selector.handle_uci_ind_pdu(next_sl_tx, make_f0_or_f1_pdu(second_rnti, {mac_harq_ack_report_status::ack}));
  auto first_rnti_action =
      selector.handle_uci_ind_pdu(next_sl_tx, make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack}));

  ASSERT_TRUE(second_rnti_action.has_value());
  ASSERT_TRUE(first_rnti_action.has_value());
}

TEST_F(uci_indication_selector_test, timeout_is_triggered_if_only_one_of_two_pucch_ucis_is_received)
{
  selector.handle_result(next_sl_tx,
                         make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2, true),
                                            make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2)}));

  auto first_action = selector.handle_uci_ind_pdu(
      next_sl_tx++, make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack, mac_harq_ack_report_status::nack}));
  ASSERT_FALSE(first_action.has_value());

  // Event: Advance slots until short timeout trigger.
  for (slot_point slot_timeout = next_sl_tx + uci_indication_selector::SHORT_PUCCH_TIMEOUT_SLOTS;
       next_sl_tx != slot_timeout;
       ++next_sl_tx) {
    ASSERT_TRUE(timeout_notifier.events.empty());
    selector.handle_result(next_sl_tx, make_sched_result({}));
  }

  // Test Case: Short timeout got triggered.
  ASSERT_EQ(timeout_notifier.events.size(), 1U);
  ASSERT_EQ(timeout_notifier.events.front().crnti, first_rnti);
  ASSERT_EQ(timeout_notifier.events.front().action.harq_ack_bits.size(), 2U);
  ASSERT_TRUE(timeout_notifier.events.front().action.harq_ack_bits.test(0));
  ASSERT_FALSE(timeout_notifier.events.front().action.harq_ack_bits.test(1));
  ASSERT_FALSE(timeout_notifier.events.front().action.sr_detected);

  // Test Case: Ensure timeout does not get triggered again.
  timeout_notifier.events.clear();
  for (slot_point slot_timeout = next_sl_tx + timeout_slots; next_sl_tx != slot_timeout; ++next_sl_tx) {
    selector.handle_result(next_sl_tx, make_sched_result({}));
    ASSERT_TRUE(timeout_notifier.events.empty());
  }
}

TEST_F(uci_indication_selector_test, timeout_is_not_triggered_if_second_pucch_uci_arrives_before_timeout)
{
  slot_point slot_start_tx      = next_sl_tx;
  slot_point second_pucch_sl_rx = next_sl_tx + uci_indication_selector::SHORT_PUCCH_TIMEOUT_SLOTS;

  // Event: Grant with two PUCCHs for same RNTI.
  selector.handle_result(next_sl_tx,
                         make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2, true),
                                            make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2)}));

  // Event: First PUCCH is received.
  auto first_action = selector.handle_uci_ind_pdu(
      next_sl_tx++,
      make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack, mac_harq_ack_report_status::nack}, false, 10.0F));
  ASSERT_FALSE(first_action.has_value());

  // Test Case: No trigger before short timeout.
  for (; next_sl_tx != second_pucch_sl_rx; ++next_sl_tx) {
    selector.handle_result(next_sl_tx, make_sched_result({}));
    ASSERT_TRUE(timeout_notifier.events.empty());
  }

  // Second UCI received before short timeout.
  auto second_action = selector.handle_uci_ind_pdu(
      slot_start_tx,
      make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::nack, mac_harq_ack_report_status::ack}, false, 1.0F));
  ASSERT_TRUE(second_action.has_value());
  ASSERT_TRUE(second_action->harq_ack_bits.test(0));
  ASSERT_FALSE(second_action->harq_ack_bits.test(1));

  // Test Case: timeout does not get triggered.
  for (slot_point slot_timeout = next_sl_tx + timeout_slots; next_sl_tx != slot_timeout; ++next_sl_tx) {
    selector.handle_result(next_sl_tx, make_sched_result({}));
    ASSERT_TRUE(timeout_notifier.events.empty());
  }
}

TEST_F(uci_indication_selector_test, dtx_pdu_does_not_override_decoded_action)
{
  selector.handle_result(next_sl_tx,
                         make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2, true),
                                            make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 2)}));

  auto first_action = selector.handle_uci_ind_pdu(
      next_sl_tx,
      make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack, mac_harq_ack_report_status::nack}, true, 5.0F));
  ASSERT_FALSE(first_action.has_value());

  auto second_action = selector.handle_uci_ind_pdu(
      next_sl_tx,
      make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::dtx, mac_harq_ack_report_status::dtx}, false, 20.0F));
  ASSERT_TRUE(second_action.has_value());
  ASSERT_EQ(second_action->harq_ack_bits.size(), 2U);
  ASSERT_TRUE(second_action->harq_ack_bits.test(0));
  ASSERT_FALSE(second_action->harq_ack_bits.test(1));
  ASSERT_TRUE(second_action->sr_detected);
  ASSERT_TRUE(timeout_notifier.events.empty());
}

TEST_F(uci_indication_selector_test, multiple_rntis_timeout_in_same_slot)
{
  slot_point start_sl_tx = next_sl_tx;
  selector.handle_result(next_sl_tx++,
                         make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 1),
                                            make_pucch_grant(second_rnti, pucch_format::FORMAT_1, 2)}));

  for (slot_point last_slot = next_sl_tx + timeout_slots; next_sl_tx != last_slot; ++next_sl_tx) {
    ASSERT_TRUE(timeout_notifier.events.empty());
    selector.handle_result(next_sl_tx, make_sched_result({}));
  }

  ASSERT_EQ(timeout_notifier.events.size(), 2U);

  auto first_it  = std::find_if(timeout_notifier.events.begin(), timeout_notifier.events.end(), [](const auto& event) {
    return event.crnti == first_rnti;
  });
  auto second_it = std::find_if(timeout_notifier.events.begin(), timeout_notifier.events.end(), [](const auto& event) {
    return event.crnti == second_rnti;
  });

  ASSERT_NE(first_it, timeout_notifier.events.end());
  ASSERT_NE(second_it, timeout_notifier.events.end());
  ASSERT_EQ(first_it->sl_rx, start_sl_tx);
  ASSERT_EQ(second_it->sl_rx, start_sl_tx);
  ASSERT_EQ(first_it->action.harq_ack_bits.size(), 1U);
  ASSERT_EQ(second_it->action.harq_ack_bits.size(), 2U);
  ASSERT_FALSE(first_it->action.harq_ack_bits.test(0));
  ASSERT_FALSE(second_it->action.harq_ack_bits.test(0));
  ASSERT_FALSE(second_it->action.harq_ack_bits.test(1));
}

TEST_F(uci_indication_selector_test, wrong_slot_for_correct_rnti_is_ignored)
{
  slot_point start_sl_tx = next_sl_tx;
  selector.handle_result(next_sl_tx++, make_sched_result({make_pucch_grant(first_rnti, pucch_format::FORMAT_1, 1)}));

  auto action_wrong_slot = selector.handle_uci_ind_pdu(
      start_sl_tx + 1, make_f0_or_f1_pdu(first_rnti, {mac_harq_ack_report_status::ack}, false, 10.0F));
  ASSERT_FALSE(action_wrong_slot.has_value());
  ASSERT_TRUE(timeout_notifier.events.empty());

  for (slot_point last_slot = next_sl_tx + timeout_slots; next_sl_tx != last_slot; ++next_sl_tx) {
    ASSERT_TRUE(timeout_notifier.events.empty());
    selector.handle_result(next_sl_tx, make_sched_result({}));
  }

  ASSERT_EQ(timeout_notifier.events.size(), 1U);
  ASSERT_EQ(timeout_notifier.events.front().crnti, first_rnti);
  ASSERT_EQ(timeout_notifier.events.front().sl_rx, start_sl_tx);
  ASSERT_EQ(timeout_notifier.events.front().action.harq_ack_bits.size(), 1U);
  ASSERT_FALSE(timeout_notifier.events.front().action.harq_ack_bits.test(0));
}

} // namespace
