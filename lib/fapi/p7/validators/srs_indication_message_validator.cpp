/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/srs_indication_message_validator.h"
#include "p7_validator_helpers.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/srs_indication.h"

using namespace ocudu;
using namespace fapi;

/// Validates the SRS usage property of the SRS.indication PDU, as per SCF-222 v4.0 section 3.4.10 in table
/// SRS.indication message body.
static bool validate_srs_usage(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 3;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "SRS usage", message_type_id::srs_indication, report);
}

/// Validates the report type of the SRS.indication PDU, as per SCF-222 v4.0 section 3.4.10 in table SRS.indication
/// message body.
static bool validate_report_type(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 7;
  static constexpr unsigned NO_REPORT = 255;

  // No report value is allowed.
  if (value == NO_REPORT) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Report type", message_type_id::srs_indication, report);
}

/// Validates the positioning report coordinate system for UL-AOA property of the SRS.indication PDU, as per SCF-222
/// v8.0 section 3.4.10 in table 3-209.
static bool validate_coordinate_system(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1;

  return validate_field(MIN_VALUE,
                        MAX_VALUE,
                        value,
                        "SRS Positioning Report coordinate system for UL-AOA",
                        message_type_id::srs_indication,
                        report);
}

/// Validates the positioning report the UL-AOA property of the SRS.indication PDU, as per SCF-222 v8.0  section 3.4.10
/// in table 3-209.
static bool validate_ul_aoa(float value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 3599;

  unsigned aoa_in_tenths = std::round(value * 10);

  return validate_field(
      MIN_VALUE, MAX_VALUE, aoa_in_tenths, "SRS Positioning Report UL-AOA", message_type_id::srs_indication, report);
}

/// Validates the positioning report gNB RX-RX time difference property of the SRS.indication PDU, as per SCF-222 v8.0
/// section 3.4.10 in table 3-209.
static bool validate_gnb_rx_tx_difference(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1970049;

  return validate_field(MIN_VALUE,
                        MAX_VALUE,
                        value,
                        "SRS Positioning Report coordinate system for UL-AOA",
                        message_type_id::srs_indication,
                        report);
}

/// Validates the positioning report RSRP property of the SRS.indication PDU, as per SCF-222 v8.0 section 3.4.10
/// in table 3-209.
static bool validate_srs_rsrp(float value, validator_report& report)
{
  static constexpr int MIN_VALUE = -1560;
  static constexpr int MAX_VALUE = 0;

  int rsrp_in_tenths = std::round(value * 10);

  return validate_field(
      MIN_VALUE, MAX_VALUE, rsrp_in_tenths, "SRS Positioning Report RSRP", message_type_id::srs_indication, report);
}

error_type<validator_report> ocudu::fapi::validate_srs_indication(const srs_indication_message& msg)
{
  validator_report report(msg.sfn, msg.slot);

  // Validate the SFN and slot.
  bool success = true;
  success &= validate_sfn(msg.sfn, message_type_id::srs_indication, report);
  success &= validate_slot(msg.slot, message_type_id::srs_indication, report);
  // NOTE: Control length property will not be validated.

  for (const auto& pdu : msg.pdus) {
    // NOTE: Handle property will not be validated.
    success &= validate_rnti(to_value(pdu.rnti), message_type_id::srs_indication, report);
    success &= validate_timing_advance_offset(pdu.timing_advance_offset, message_type_id::srs_indication, report);
    success &= validate_timing_advance_offset_ns(pdu.timing_advance_offset_ns, message_type_id::srs_indication, report);
    success &= validate_srs_usage(static_cast<unsigned>(pdu.usage), report);
    success &= validate_report_type(static_cast<unsigned>(pdu.report_type), report);

    // Validate positioning fields.
    if (pdu.report_type == srs_report_type::positioning) {
      const srs_positioning_report& pos_report = pdu.positioning;
      success &= validate_coordinate_system(static_cast<unsigned>(pos_report.coordinate_system_aoa), report);
      if (pos_report.gnb_rx_tx_difference) {
        success &= validate_gnb_rx_tx_difference(pos_report.gnb_rx_tx_difference.value(), report);
      }
      if (pos_report.ul_aoa) {
        success &= validate_ul_aoa(pos_report.ul_aoa.value(), report);
      }
      // NOTE: UL Relative Time Of Arrival is in PHY time units and will not be validated.

      if (pos_report.rsrp) {
        success &= validate_srs_rsrp(pos_report.rsrp.value(), report);
      }
    }
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
