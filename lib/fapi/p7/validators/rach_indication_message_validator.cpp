/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/rach_indication_message_validator.h"
#include "p7_validator_helpers.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/rach_indication.h"

using namespace ocudu;
using namespace fapi;

/// Validates the symbol index property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_symbol_index(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 13;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Symbol index", message_type_id::rach_indication, report);
}

/// Validates the slot index property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_slot_index(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 79;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Slot index", message_type_id::rach_indication, report);
}

/// Validates the RACH index property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_ra_index(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 7;

  return validate_field(MIN_VALUE,
                        MAX_VALUE,
                        value,
                        "Index of the received PRACH frequency domain occasion",
                        message_type_id::rach_indication,
                        report);
}

/// Validates the AVG RSSI property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_avg_rssi(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 170000;

  if (value == std::numeric_limits<uint32_t>::max()) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "AVG RSSI", message_type_id::rach_indication, report);
}

/// Validates the RSRP property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_rach_rsrp(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1280;

  if (value == std::numeric_limits<uint16_t>::max()) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "RSRP", message_type_id::rach_indication, report);
}

/// Validates the preamble index property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_preamble_index(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 63;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Preamble index", message_type_id::rach_indication, report);
}

/// Validates the timing advance offset of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_rach_timing_advance_offset(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 3846;

  if (value == std::numeric_limits<uint16_t>::max()) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Timing advance offset", message_type_id::rach_indication, report);
}

/// Validates the preamble power property of the RACH.indication PDU, as per SCF-222 v4.0 section 3.4.11 in table
/// RACH.indication message body.
static bool validate_preamble_power(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 170000;

  if (value == std::numeric_limits<uint32_t>::max()) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Preamble power", message_type_id::rach_indication, report);
}

error_type<validator_report> ocudu::fapi::validate_rach_indication(const rach_indication& msg)
{
  validator_report report(msg.slot);

  bool success = true;

  for (const auto& pdu : msg.pdus) {
    success &= validate_symbol_index(pdu.symbol_index, report);
    success &= validate_slot_index(pdu.slot_index, report);
    success &= validate_ra_index(pdu.ra_index, report);
    success &= validate_avg_rssi(pdu.avg_rssi, report);
    success &= validate_rach_rsrp(pdu.rsrp, report);
    // NOTE: AVG SNR property uses the whole range of the property, so it will not be validated.
    for (const auto& preamble : pdu.preambles) {
      success &= validate_preamble_index(preamble.preamble_index, report);
      success &= validate_rach_timing_advance_offset(preamble.timing_advance_offset, report);
      success &= validate_timing_advance_offset_ns(preamble.timing_advance_offset_ns, report);
      success &= validate_preamble_power(preamble.preamble_pwr, report);
      // NOTE: Preamble SNR property uses the whole range of the property, so it will not be validated.
    }
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
