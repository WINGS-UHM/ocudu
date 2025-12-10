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
#include "ocudu/fapi/p7/validators/srs_indication_message_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;

class validate_srs_indication_field
  : public validate_fapi_message<srs_indication>,
    public testing::TestWithParam<std::tuple<pdu_field_data<srs_indication>, test_case_data>>
{};

TEST_P(validate_srs_indication_field, WithValue)
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
               build_valid_srs_indication,
               validate_srs_indication,
               ocudu::fapi::message_type_id::srs_indication);
}

INSTANTIATE_TEST_SUITE_P(
    sfn,
    validate_srs_indication_field,
    testing::Combine(testing::Values(pdu_field_data<srs_indication>{"sfn",
                                                                    [](srs_indication& pdu, int value) {
                                                                      pdu.slot = slot_point(
                                                                          subcarrier_spacing::kHz240, value, 0);
                                                                    }}),
                     testing::Values(test_case_data{0, true}, test_case_data{512, true}, test_case_data{1023, true})));

INSTANTIATE_TEST_SUITE_P(
    slot,
    validate_srs_indication_field,
    testing::Combine(testing::Values(pdu_field_data<srs_indication>{"slot",
                                                                    [](srs_indication& pdu, int value) {
                                                                      pdu.slot = slot_point(
                                                                          subcarrier_spacing::kHz240, 0, value);
                                                                    }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));

INSTANTIATE_TEST_SUITE_P(RNTI,
                         validate_srs_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<srs_indication>{
                                              "RNTI",
                                              [](srs_indication& pdu, int value) {
                                                pdu.pdus.back().rnti = to_rnti(value);
                                              }}),
                                          testing::Values(test_case_data{0, false},
                                                          test_case_data{1, true},
                                                          test_case_data{32767, true},
                                                          test_case_data{65535, true})));

INSTANTIATE_TEST_SUITE_P(
    ta,
    validate_srs_indication_field,
    testing::Combine(testing::Values(pdu_field_data<srs_indication>{
                         "Timing advance offset",
                         [](srs_indication& msg, int value) { msg.pdus.back().timing_advance_offset = value; }}),
                     testing::Values(test_case_data{0, true},
                                     test_case_data{32, true},
                                     test_case_data{63, true},
                                     test_case_data{64, false},
                                     test_case_data{std::numeric_limits<uint16_t>::max() - 1, false},
                                     test_case_data{std::numeric_limits<uint16_t>::max(), true})));
INSTANTIATE_TEST_SUITE_P(ta_ns,
                         validate_srs_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<srs_indication>{
                                              "Timing advance offset in nanoseconds",
                                              [](srs_indication& msg, int value) {
                                                msg.pdus.back().timing_advance_offset_ns = value;
                                              }}),
                                          testing::Values(test_case_data{static_cast<unsigned>(int16_t(-10000)), true},
                                                          test_case_data{static_cast<unsigned>(int16_t(-16800)), true},
                                                          test_case_data{10000, true},
                                                          test_case_data{16801, false},
                                                          test_case_data{static_cast<unsigned>(int16_t(-16801)), false},
                                                          test_case_data{static_cast<unsigned>(int16_t(-32767)), false},
                                                          test_case_data{std::numeric_limits<uint16_t>::max(), true})));

/// Valid Message should pass.
TEST(validate_srs_indication, valid_indication_passes)
{
  auto msg = build_valid_srs_indication();

  const auto& result = validate_srs_indication(msg);

  EXPECT_TRUE(result);
}

/// Add 3 errors and check that validation fails with 3 errors.
TEST(validate_srs_indication, invalid_indication_fails)
{
  auto msg = build_valid_srs_indication();

  msg.pdus.back().timing_advance_offset    = 64U;
  msg.pdus.back().timing_advance_offset_ns = 17000U;
  msg.pdus.back().report_type              = static_cast<srs_report_type>(28);

  const auto& result = validate_srs_indication(msg);

  EXPECT_FALSE(result);
  const auto& report = result.error();
  // Check that the 3 errors are reported.
  EXPECT_EQ(report.reports.size(), 3U);
}

#ifdef ASSERTS_ENABLED
INSTANTIATE_TEST_SUITE_P(invalid_sfn,
                         validate_srs_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<srs_indication>{
                                              "sfn",
                                              [](srs_indication& pdu, int value) {
                                                pdu.slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));

INSTANTIATE_TEST_SUITE_P(invalid_slot,
                         validate_srs_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<srs_indication>{
                                              "slot",
                                              [](srs_indication& pdu, int value) {
                                                pdu.slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
