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
#include "ocudu/fapi/p7/validators/tx_data_request_message_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;

class validate_tx_data_request_field
  : public validate_fapi_message<tx_data_request>,
    public testing::TestWithParam<std::tuple<pdu_field_data<tx_data_request>, test_case_data>>
{};

TEST_P(validate_tx_data_request_field, with_value)
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
               build_valid_tx_data_request,
               validate_tx_data_request,
               ocudu::fapi::message_type_id::tx_data_request);
}

INSTANTIATE_TEST_SUITE_P(
    sfn,
    validate_tx_data_request_field,
    testing::Combine(testing::Values(pdu_field_data<tx_data_request>{"sfn",
                                                                     [](tx_data_request& msg, int value) {
                                                                       msg.slot = slot_point(
                                                                           subcarrier_spacing::kHz240, value, 0);
                                                                     }}),
                     testing::Values(test_case_data{0, true}, test_case_data{512, true}, test_case_data{1023, true})));

INSTANTIATE_TEST_SUITE_P(
    slot,
    validate_tx_data_request_field,
    testing::Combine(testing::Values(pdu_field_data<tx_data_request>{"slot",
                                                                     [](tx_data_request& msg, int value) {
                                                                       msg.slot = slot_point(
                                                                           subcarrier_spacing::kHz240, 0, value);
                                                                     }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));

INSTANTIATE_TEST_SUITE_P(
    cw_index,
    validate_tx_data_request_field,
    testing::Combine(testing::Values(pdu_field_data<tx_data_request>{
                         "CW index",
                         [](tx_data_request& msg, int value) { msg.pdus.back().cw_index = value; }}),
                     testing::Values(test_case_data{0, true}, test_case_data{1, true}, test_case_data{2, false})));

TEST(validate_tx_data_request_pdu, valid_pdu_passes)
{
  auto msg = build_valid_tx_data_request();

  const auto& result = validate_tx_data_request(msg);

  ASSERT_TRUE(result);
}

TEST(validate_tx_data_request_pdu, invalid_pdu_fails)
{
  auto msg = build_valid_tx_data_request();

  auto     scs             = subcarrier_spacing::kHz240;
  unsigned sfn             = 100U;
  auto     slot_index      = 0U;
  msg.slot                 = slot_point(scs, sfn, slot_index);
  msg.pdus.back().cw_index = 3;

  const auto& result = validate_tx_data_request(msg);

  ASSERT_FALSE(result);

  const auto& report = result.error();
  // Check that the 3 errors are reported.
  ASSERT_EQ(report.reports.size(), 1u);
}

#ifdef ASSERTS_ENABLED
INSTANTIATE_TEST_SUITE_P(invalid_sfn,
                         validate_tx_data_request_field,
                         testing::Combine(testing::Values(pdu_field_data<tx_data_request>{
                                              "sfn",
                                              [](tx_data_request& msg, int value) {
                                                msg.slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));

INSTANTIATE_TEST_SUITE_P(invalid_slot,
                         validate_tx_data_request_field,
                         testing::Combine(testing::Values(pdu_field_data<tx_data_request>{
                                              "slot",
                                              [](tx_data_request& msg, int value) {
                                                msg.slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
