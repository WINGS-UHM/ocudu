/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/rx_data_indication_message_validator.h"
#include "p7_validator_helpers.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"

using namespace ocudu;
using namespace fapi;

/// Validates the PDU tag property of the Rx_Data.indication PDU, as per SCF-222 v4.0 section 3.4.7 in table
/// Rx_Data.indication message body.
/// \note This validator only accepts the custom tag.
static bool validate_pdu_tag(rx_data_indication_pdu::pdu_tag_type value, validator_report& report)
{
  if (value == rx_data_indication_pdu::pdu_tag_type::custom) {
    return true;
  }

  report.append(static_cast<int>(value), "PDU tag", message_type_id::rx_data_indication);

  return false;
}

/// Validates the PDU value property of the Rx_Data.indication PDU, as per SCF-222 v4.0 section 3.4.7 in table
/// Rx_Data.indication message body.
static bool validate_pdu_value(const uint8_t* value, validator_report& report)
{
  if (value != nullptr) {
    return true;
  }

  report.append(0, "PDU tag", message_type_id::rx_data_indication);

  return false;
}

error_type<validator_report> ocudu::fapi::validate_rx_data_indication(const rx_data_indication_message& msg)
{
  validator_report report(msg.sfn, msg.slot);

  static constexpr message_type_id msg_id = message_type_id::rx_data_indication;

  // Validate the SFN and slot.
  bool success = true;
  success &= validate_sfn(msg.sfn, msg_id, report);
  success &= validate_slot(msg.slot, msg_id, report);
  // NOTE: Control length property will not be validated.

  for (const auto& pdu : msg.pdus) {
    // NOTE: Handle property will not be validated.
    success &= validate_rnti(to_value(pdu.rnti), msg_id, report);
    success &= validate_rapid(pdu.rapid, msg_id, report);
    success &= validate_harq_id(pdu.harq_id, msg_id, report);
    // NOTE: PDU length property will not be validated.
    success &= validate_pdu_tag(pdu.pdu_tag, report);
    // PDUs that were not decoded successfully do not carry data.
    if (pdu.pdu_length) {
      success &= validate_pdu_value(pdu.data, report);
    }
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
