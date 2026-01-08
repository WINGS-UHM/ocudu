/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "../../helpers.h"
#include "../../message_builder_helpers.h"
#include "ocudu/fapi/p7/validators/rx_data_indication_message_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;

class validate_rx_data_indication_field
  : public validate_fapi_message<rx_data_indication>,
    public testing::TestWithParam<std::tuple<pdu_field_data<rx_data_indication>, test_case_data>>
{};

TEST_P(validate_rx_data_indication_field, WithValue)
{
  auto params   = GetParam();
  auto property = std::get<0>(params).property;
  auto value    = std::get<1>(params).value;

  if (property == "sfn" && value >= 1024) {
    const char* expected_regex = R"((.*)Invalid SFN(.*))";
    ASSERT_DEATH(slot_point(subcarrier_spacing::kHz240, value, 0), expected_regex);
    return;
  }

  if (property == "slot" && value >= 160) {
    const char* expected_regex = R"((.*)Slot index(.*) exceeds maximum number of slots(.*))";
    ASSERT_DEATH(slot_point(subcarrier_spacing::kHz240, 0, value), expected_regex);
    return;
  }

  execute_test(std::get<0>(params),
               std::get<1>(params),
               build_valid_rx_data_indication,
               validate_rx_data_indication,
               ocudu::fapi::message_type_id::rx_data_indication);
}

INSTANTIATE_TEST_SUITE_P(
    sfn,
    validate_rx_data_indication_field,
    testing::Combine(testing::Values(pdu_field_data<rx_data_indication>{"sfn",
                                                                        [](rx_data_indication& pdu, int value) {
                                                                          pdu.slot = slot_point(
                                                                              subcarrier_spacing::kHz240, value, 0);
                                                                        }}),
                     testing::Values(test_case_data{0, true}, test_case_data{512, true}, test_case_data{1023, true})));

INSTANTIATE_TEST_SUITE_P(
    slot,
    validate_rx_data_indication_field,
    testing::Combine(testing::Values(pdu_field_data<rx_data_indication>{"slot",
                                                                        [](rx_data_indication& pdu, int value) {
                                                                          pdu.slot = slot_point(
                                                                              subcarrier_spacing::kHz240, 0, value);
                                                                        }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));

INSTANTIATE_TEST_SUITE_P(RNTI,
                         validate_rx_data_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rx_data_indication>{
                                              "RNTI",
                                              [](rx_data_indication& pdu, int value) {
                                                pdu.pdus.back().rnti = to_rnti(value);
                                              }}),
                                          testing::Values(test_case_data{0, false},
                                                          test_case_data{1, true},
                                                          test_case_data{32767, true},
                                                          test_case_data{65535, true})));

INSTANTIATE_TEST_SUITE_P(HarqID,
                         validate_rx_data_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rx_data_indication>{
                                              "HARQ ID",
                                              [](rx_data_indication& pdu, int value) {
                                                pdu.pdus.back().harq_id = to_harq_id(value);
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{8, true},
                                                          test_case_data{15, true},
                                                          test_case_data{16, true},
                                                          test_case_data{24, true},
                                                          test_case_data{31, true},
                                                          test_case_data{32, false})));

/// Valid Message should pass.
TEST(validate_rx_data_indication, valid_indication_passes)
{
  auto msg = build_valid_rx_data_indication();

  const auto& result = validate_rx_data_indication(msg);

  EXPECT_TRUE(result);
}

/// Add 3 errors and check that validation fails with 3 errors.
TEST(validate_rx_data_indication, invalid_indication_fails)
{
  auto msg = build_valid_rx_data_indication();

  msg.pdus.back().harq_id = to_harq_id(32U);

  const auto& result = validate_rx_data_indication(msg);

  EXPECT_FALSE(result);
  const auto& report = result.error();
  // Check that the 3 errors are reported.
  EXPECT_EQ(report.reports.size(), 1U);
}

#ifdef ASSERTS_ENABLED
INSTANTIATE_TEST_SUITE_P(invalid_sfn,
                         validate_rx_data_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rx_data_indication>{
                                              "sfn",
                                              [](rx_data_indication& pdu, int value) {
                                                pdu.slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));

INSTANTIATE_TEST_SUITE_P(invalid_slot,
                         validate_rx_data_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rx_data_indication>{
                                              "slot",
                                              [](rx_data_indication& pdu, int value) {
                                                pdu.slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
