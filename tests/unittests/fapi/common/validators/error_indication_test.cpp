
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
#include "ocudu/fapi/common/error_indication_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;
class validate_error_indication_field
  : public validate_fapi_message<error_indication>,
    public testing::TestWithParam<std::tuple<pdu_field_data<error_indication>, test_case_data>>
{};
TEST_P(validate_error_indication_field, WithValue)
{
  auto params   = GetParam();
  auto property = std::get<0>(params).property;
  auto value    = std::get<1>(params).value;

  if (property == "Expected SFN" && value >= 1024) {
    const char* expected_regex = R"((.*)Invalid SFN(.*))";
    ASSERT_DEATH(slot_point(subcarrier_spacing::kHz240, value, 0), expected_regex);
    return;
  }

  if (property == "Expected slot" && value >= 160) {
    const char* expected_regex = R"((.*)Slot index(.*) exceeds maximum number of slots(.*))";
    ASSERT_DEATH(slot_point(subcarrier_spacing::kHz240, 0, value), expected_regex);
    return;
  }

  execute_test(std::get<0>(params),
               std::get<1>(params),
               build_valid_error_indication,
               validate_error_indication,
               ocudu::fapi::message_type_id::error_indication);
}
INSTANTIATE_TEST_SUITE_P(MessageID,
                         validate_error_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<error_indication>{
                                              "Message ID",
                                              [](error_indication& msg, int value) {
                                                msg.message_id = static_cast<message_type_id>(value);
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{7, true},
                                                          test_case_data{8, false},
                                                          test_case_data{0x4f, false},
                                                          test_case_data{0x7f, false},
                                                          test_case_data{0x80, true},
                                                          test_case_data{0x8a, true},
                                                          test_case_data{0x8b, false},
                                                          test_case_data{0xff, false},
                                                          test_case_data{0x179, false},
                                                          test_case_data{0x180, true},
                                                          test_case_data{0x181, true},
                                                          test_case_data{0x182, true},
                                                          test_case_data{0x183, false})));
INSTANTIATE_TEST_SUITE_P(ErrorCodeID,
                         validate_error_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<error_indication>{
                                              "Error code",
                                              [](error_indication& msg, int value) {
                                                msg.error_code = static_cast<error_code_id>(value);

                                                if (msg.error_code == error_code_id::msg_ok) {
                                                  msg.expected_slot = std::nullopt;
                                                }
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{8, true},
                                                          test_case_data{0xc, true},
                                                          test_case_data{0xd, false})));
INSTANTIATE_TEST_SUITE_P(
    ExpectedSFN,
    validate_error_indication_field,
    testing::Combine(testing::Values(pdu_field_data<error_indication>{"Expected SFN",
                                                                      [](error_indication& msg, int value) {
                                                                        msg.error_code = error_code_id::out_of_sync;

                                                                        msg.expected_slot = slot_point(
                                                                            subcarrier_spacing::kHz240, value, 0);
                                                                      }}),
                     testing::Values(test_case_data{0, true}, test_case_data{522, true}, test_case_data{1023, true})));
INSTANTIATE_TEST_SUITE_P(
    ExpectedSlot,
    validate_error_indication_field,
    testing::Combine(testing::Values(pdu_field_data<error_indication>{"Expected slot",
                                                                      [](error_indication& msg, int value) {
                                                                        msg.error_code    = error_code_id::out_of_sync;
                                                                        msg.expected_slot = slot_point(
                                                                            subcarrier_spacing::kHz240, 0, value);
                                                                      }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));
TEST(validate_error_indication, valid_message_passes)
{
  const auto& msg    = build_valid_error_indication();
  const auto& result = validate_error_indication(msg);
  ASSERT_TRUE(result);
}
TEST(validate_out_of_sync_error_indication, valid_message_passes)
{
  const auto& msg    = build_valid_out_of_sync_error_indication();
  const auto& result = validate_error_indication(msg);
  ASSERT_TRUE(result);
}

#ifdef ASSERTS_ENABLED
TEST(validate_invalid_sfn_error_indication, valid_message_passes)
{
  const char* expected_regex = R"((.*)Invalid SFN(.*))";
  ASSERT_DEATH(build_valid_invalid_sfn_error_indication(), expected_regex);
}

TEST(validate_out_of_sync_error_indication, invalid_message_fails)
{
  const char* expected_regex = R"((.*)Invalid SFN(.*))";
  ASSERT_DEATH(build_valid_tx_err_error_indication(), expected_regex);
}

INSTANTIATE_TEST_SUITE_P(ExpectedInvalidSFN,
                         validate_error_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<error_indication>{
                                              "Expected SFN",
                                              [](error_indication& msg, int value) {
                                                msg.error_code = error_code_id::out_of_sync;

                                                msg.expected_slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));
INSTANTIATE_TEST_SUITE_P(ExpectedInvalidSlot,
                         validate_error_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<error_indication>{
                                              "Expected slot",
                                              [](error_indication& msg, int value) {
                                                msg.error_code    = error_code_id::out_of_sync;
                                                msg.expected_slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
