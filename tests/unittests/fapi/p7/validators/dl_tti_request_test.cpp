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
#include "ocudu/fapi/p7/validators/dl_tti_request_message_validator.h"

using namespace ocudu;
using namespace fapi;
using namespace unittest;

class validate_dl_tti_request_field
  : public validate_fapi_message<dl_tti_request>,
    public testing::TestWithParam<std::tuple<pdu_field_data<dl_tti_request>, test_case_data>>
{};

TEST_P(validate_dl_tti_request_field, with_value)
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
               build_valid_dl_tti_request,
               validate_dl_tti_request,
               ocudu::fapi::message_type_id::dl_tti_request);
}

INSTANTIATE_TEST_SUITE_P(
    sfn,
    validate_dl_tti_request_field,
    testing::Combine(testing::Values(pdu_field_data<dl_tti_request>{"sfn",
                                                                    [](dl_tti_request& msg, int value) {
                                                                      msg.slot = slot_point(
                                                                          subcarrier_spacing::kHz240, value, 0);
                                                                    }}),
                     testing::Values(test_case_data{0, true}, test_case_data{512, true}, test_case_data{1023, true})));

INSTANTIATE_TEST_SUITE_P(
    slot,
    validate_dl_tti_request_field,
    testing::Combine(testing::Values(pdu_field_data<dl_tti_request>{"slot",
                                                                    [](dl_tti_request& msg, int value) {
                                                                      msg.slot = slot_point(
                                                                          subcarrier_spacing::kHz240, 0, value);
                                                                    }}),
                     testing::Values(test_case_data{0, true}, test_case_data{80, true}, test_case_data{159, true})));

/// Tests that a valid UL_TTI.request message validates correctly.
TEST(validate_dl_tti_request, valid_request_passes)
{
  dl_tti_request msg = build_valid_dl_tti_request();

  const auto& result = validate_dl_tti_request(msg);

  ASSERT_TRUE(result);
}

/// Tests that a UL_TTI.request message which contains errors in some properties fails to validate.
TEST(validate_dl_tti_request, invalid_request_fails)
{
  dl_tti_request msg = build_valid_dl_tti_request();

  // Set some errors.
  msg.pdus[0].ssb_pdu.phys_cell_id       = 2000;
  msg.pdus[1].pdcch_pdu.coreset_bwp_size = 2000;
  msg.pdus[2].pdsch_pdu.bwp_size         = 2000;
  msg.pdus[3].csi_rs_pdu.scramb_id       = 2000;

  const auto& result = validate_dl_tti_request(msg);

  ASSERT_FALSE(result);

  const auto& report = result.error();
  // Check that the 4 errors are reported.
  ASSERT_EQ(report.reports.size(), 4u);
}

#ifdef ASSERTS_ENABLED
INSTANTIATE_TEST_SUITE_P(invalid_sfn,
                         validate_dl_tti_request_field,
                         testing::Combine(testing::Values(pdu_field_data<dl_tti_request>{
                                              "sfn",
                                              [](dl_tti_request& msg, int value) {
                                                msg.slot = slot_point(subcarrier_spacing::kHz240, value, 0);
                                              }}),
                                          testing::Values(test_case_data{1024, false})));

INSTANTIATE_TEST_SUITE_P(invalid_slot,
                         validate_dl_tti_request_field,
                         testing::Combine(testing::Values(pdu_field_data<dl_tti_request>{
                                              "slot",
                                              [](dl_tti_request& msg, int value) {
                                                msg.slot = slot_point(subcarrier_spacing::kHz240, 0, value);
                                              }}),
                                          testing::Values(test_case_data{160, false})));
#endif
