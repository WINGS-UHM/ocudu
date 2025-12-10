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
#include "ocudu/fapi/p7/validators/slot_indication_message_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;

class validate_slot_indication_field
  : public validate_fapi_message<slot_indication>,
    public testing::TestWithParam<std::tuple<pdu_field_data<slot_indication>, test_case_data>>
{};

TEST_P(validate_slot_indication_field, WithValue)
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
               build_valid_slot_indication,
               validate_slot_indication,
               ocudu::fapi::message_type_id::slot_indication);
}

INSTANTIATE_TEST_SUITE_P(
    SFN,
    validate_slot_indication_field,
    testing::Combine(testing::Values(pdu_field_data<slot_indication>{"sfn",
                                                                     [](slot_indication& msg, int value) {
                                                                       msg.slot = slot_point(
                                                                           subcarrier_spacing::kHz240, value, 0);
                                                                     }}),
                     testing::Values(test_case_data{0, true}, test_case_data{522, true}, test_case_data{1023, true})));

INSTANTIATE_TEST_SUITE_P(
    slot,
    validate_slot_indication_field,
    testing::Combine(testing::Values(pdu_field_data<slot_indication>{"slot",
                                                                     [](slot_indication& msg, int value) {
                                                                       msg.slot = slot_point(
                                                                           subcarrier_spacing::kHz240, 0, value);
                                                                     }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));

TEST(validate_slot_indication, valid_message_passes)
{
  const auto& msg = build_valid_slot_indication();

  const auto& result = validate_slot_indication(msg);

  ASSERT_TRUE(result);
}

#ifdef ASSERTS_ENABLED
INSTANTIATE_TEST_SUITE_P(invalidSFN,
                         validate_slot_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<slot_indication>{
                                              "sfn",
                                              [](slot_indication& msg, int value) {
                                                msg.slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));

INSTANTIATE_TEST_SUITE_P(invalid_slot,
                         validate_slot_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<slot_indication>{
                                              "slot",
                                              [](slot_indication& msg, int value) {
                                                msg.slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
