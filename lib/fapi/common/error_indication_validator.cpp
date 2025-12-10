/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/common/error_indication_validator.h"
#include "validator_helpers.h"
#include "ocudu/fapi/common/error_indication.h"

using namespace ocudu;
using namespace fapi;

/// Validates the slot property of a message.
inline bool validate_slot(std::optional<slot_point> slot, validator_report& report)
{
  // If the slot is an std::nullopt, then it is valid.
  if (!slot) {
    return true;
  }

  // If the slot is valid, stop cheking.
  if (slot->valid()) {
    return true;
  }

  report.append(static_cast<int>(slot->slot_index()), "Slot", message_type_id::error_indication);

  return false;
}

/// Validates the message id property of the ERROR.indication PDU, as per SCF-222 v4.0 section 3.3.6.1 in table
/// ERROR.indication message body.
static bool validate_message_id(unsigned value, validator_report& report)
{
  // Check first range.
  static constexpr unsigned MIN_VALUE_FIRST_RANGE = 0U;
  static constexpr unsigned MAX_VALUE_FIRST_RANGE = 7U;

  if (MIN_VALUE_FIRST_RANGE <= value && value <= MAX_VALUE_FIRST_RANGE) {
    return true;
  }

  // Check second range.
  static constexpr unsigned MIN_VALUE_SECOND_RANGE = 0x80U;
  static constexpr unsigned MAX_VALUE_SECOND_RANGE = 0x8aU;

  if (MIN_VALUE_SECOND_RANGE <= value && value <= MAX_VALUE_SECOND_RANGE) {
    return true;
  }

  // Check third range.
  static constexpr unsigned MIN_VALUE_THIRD_RANGE = 0x180U;
  static constexpr unsigned MAX_VALUE_THIRD_RANGE = 0x182U;

  if (MIN_VALUE_THIRD_RANGE <= value && value <= MAX_VALUE_THIRD_RANGE) {
    return true;
  }

  report.append(static_cast<int>(value), "Message ID", message_type_id::error_indication);

  return false;
}

/// Validates the error code property of the ERROR.indication PDU, as per SCF-222 v4.0 section 3.3.6.1 in table
/// ERROR.indication message body.
static bool validate_error_code(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0U;
  static constexpr unsigned MAX_VALUE = 0xcU;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Error code", message_type_id::error_indication, report);
}

/// Validates the expected slot property of the ERROR.indication PDU, as per SCF-222 v4.0 section 3.3.6.1 in table
/// ERROR.indication message body.
static bool validate_expected_slot(std::optional<slot_point> slot, error_code_id error_code, validator_report& report)
{
  auto add_to_report_and_return_false = [&report, &slot]() {
    report.append(static_cast<int>(slot->slot_index()), "Expected slot", message_type_id::error_indication);
    return false;
  };

  // If there is no error, the expected slot should be std::nullotp.
  if (error_code == error_code_id::msg_ok) {
    if (!slot) {
      return true;
    }

    return add_to_report_and_return_false();
  }

  // If there is error, the expected slot should not be std::nullotp and should be valid.
  if (!slot) {
    return add_to_report_and_return_false();
  }

  if (slot->valid()) {
    return true;
  }

  return add_to_report_and_return_false();
}

error_type<validator_report> ocudu::fapi::validate_error_indication(const error_indication& msg)
{
  auto             slot = msg.slot ? msg.slot.value() : slot_point();
  validator_report report(slot);

  bool success = true;
  success &= validate_slot(slot, report);
  success &= validate_message_id(static_cast<unsigned>(msg.message_id), report);
  success &= validate_error_code(static_cast<unsigned>(msg.error_code), report);
  success &= validate_expected_slot(msg.expected_slot, msg.error_code, report);

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
