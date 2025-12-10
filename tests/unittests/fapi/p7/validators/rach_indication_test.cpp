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
#include "ocudu/fapi/p7/validators/rach_indication_message_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;

class validate_rach_indication_field
  : public validate_fapi_message<rach_indication>,
    public testing::TestWithParam<std::tuple<pdu_field_data<rach_indication>, test_case_data>>
{};

TEST_P(validate_rach_indication_field, with_value)
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
               build_valid_rach_indication,
               validate_rach_indication,
               ocudu::fapi::message_type_id::rach_indication);
}

INSTANTIATE_TEST_SUITE_P(
    sfn,
    validate_rach_indication_field,
    testing::Combine(testing::Values(pdu_field_data<rach_indication>{"sfn",
                                                                     [](rach_indication& pdu, int value) {
                                                                       pdu.slot = slot_point(
                                                                           subcarrier_spacing::kHz240, value, 0);
                                                                     }}),
                     testing::Values(test_case_data{0, true}, test_case_data{512, true}, test_case_data{1023, true})));

INSTANTIATE_TEST_SUITE_P(
    slot,
    validate_rach_indication_field,
    testing::Combine(testing::Values(pdu_field_data<rach_indication>{"slot",
                                                                     [](rach_indication& pdu, int value) {
                                                                       pdu.slot = slot_point(
                                                                           subcarrier_spacing::kHz240, 0, value);
                                                                     }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));

INSTANTIATE_TEST_SUITE_P(symbol_index,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "Symbol index",
                                              [](rach_indication& msg, int value) {
                                                msg.pdus.back().symbol_index = value;
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{6, true},
                                                          test_case_data{13, true},
                                                          test_case_data{14, false})));

INSTANTIATE_TEST_SUITE_P(slot_index,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "Slot index",
                                              [](rach_indication& msg, int value) {
                                                msg.pdus.back().slot_index = value;
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{40, true},
                                                          test_case_data{79, true},
                                                          test_case_data{80, false})));

INSTANTIATE_TEST_SUITE_P(ra_index,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "Index of the received PRACH frequency domain occasion",
                                              [](rach_indication& msg, int value) {
                                                msg.pdus.back().ra_index = value;
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{4, true},
                                                          test_case_data{7, true},
                                                          test_case_data{8, false})));

INSTANTIATE_TEST_SUITE_P(
    rssi,
    validate_rach_indication_field,
    testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                         "AVG RSSI",
                         [](rach_indication& msg, int value) { msg.pdus.back().avg_rssi = value; }}),
                     testing::Values(test_case_data{0, true},
                                     test_case_data{85000, true},
                                     test_case_data{170000, true},
                                     test_case_data{170001, false},
                                     test_case_data{std::numeric_limits<uint32_t>::max() - 1, false},
                                     test_case_data{std::numeric_limits<uint32_t>::max(), true})));

INSTANTIATE_TEST_SUITE_P(rsrp,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "RSRP",
                                              [](rach_indication& msg, int value) { msg.pdus.back().rsrp = value; }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{640, true},
                                                          test_case_data{1280, true},
                                                          test_case_data{1281, false},
                                                          test_case_data{std::numeric_limits<uint16_t>::max() - 1,
                                                                         false},
                                                          test_case_data{std::numeric_limits<uint16_t>::max(), true})));

INSTANTIATE_TEST_SUITE_P(preamble_index,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "Preamble index",
                                              [](rach_indication& msg, int value) {
                                                msg.pdus.back().preambles.back().preamble_index = value;
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{32, true},
                                                          test_case_data{63, true},
                                                          test_case_data{64, false})));

INSTANTIATE_TEST_SUITE_P(ta,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "Timing advance offset",
                                              [](rach_indication& msg, int value) {
                                                msg.pdus.back().preambles.back().timing_advance_offset = value;
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{1923, true},
                                                          test_case_data{3846, true},
                                                          test_case_data{3847, false},
                                                          test_case_data{std::numeric_limits<uint16_t>::max() - 1,
                                                                         false},
                                                          test_case_data{std::numeric_limits<uint16_t>::max(), true})));
INSTANTIATE_TEST_SUITE_P(ta_ns,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "Timing advance offset in nanoseconds",
                                              [](rach_indication& msg, int value) {
                                                msg.pdus.back().preambles.back().timing_advance_offset_ns = value;
                                              }}),
                                          testing::Values(test_case_data{0, true},
                                                          test_case_data{1002000, true},
                                                          test_case_data{2005000, true},
                                                          test_case_data{2005001, false},
                                                          test_case_data{std::numeric_limits<uint32_t>::max() - 1,
                                                                         false},
                                                          test_case_data{std::numeric_limits<uint32_t>::max(), true})));

INSTANTIATE_TEST_SUITE_P(
    preamble_pwr,
    validate_rach_indication_field,
    testing::Combine(testing::Values(pdu_field_data<rach_indication>{"Preamble power",
                                                                     [](rach_indication& msg, int value) {
                                                                       msg.pdus.back().preambles.back().preamble_pwr =
                                                                           value;
                                                                     }}),
                     testing::Values(test_case_data{0, true},
                                     test_case_data{85000, true},
                                     test_case_data{170000, true},
                                     test_case_data{170001, false},
                                     test_case_data{std::numeric_limits<uint32_t>::max() - 1, false},
                                     test_case_data{std::numeric_limits<uint32_t>::max(), true})));

/// Valid Message should pass.
TEST(validate_rach_indication, valid_indication_passes)
{
  rach_indication msg = build_valid_rach_indication();

  const auto& result = validate_rach_indication(msg);

  ASSERT_TRUE(result);
}

TEST(validate_rach_indication, invalid_indication_fails)
{
  rach_indication msg = build_valid_rach_indication();

  msg.pdus.back().symbol_index = 14;
  msg.pdus.back().slot_index   = 80;
  msg.pdus.back().ra_index     = 9;

  const auto& result = validate_rach_indication(msg);

  ASSERT_FALSE(result);
  const auto& report = result.error();
  // Check that the 3 errors are reported.
  ASSERT_EQ(report.reports.size(), 3U);
}

#ifdef ASSERTS_ENABLED
INSTANTIATE_TEST_SUITE_P(invalid_sfn,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "sfn",
                                              [](rach_indication& pdu, int value) {
                                                pdu.slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));

INSTANTIATE_TEST_SUITE_P(invalid_slot,
                         validate_rach_indication_field,
                         testing::Combine(testing::Values(pdu_field_data<rach_indication>{
                                              "slot",
                                              [](rach_indication& pdu, int value) {
                                                pdu.slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
