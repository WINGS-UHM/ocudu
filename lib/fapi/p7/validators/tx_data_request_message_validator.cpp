/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/tx_data_request_message_validator.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/tx_data_request.h"

using namespace ocudu;
using namespace fapi;

/// Validates the CW index property of a Tx_Data.request message, as per SCF-222 v4.0 section 3.4.6.
static bool validate_pdu_cw_index(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "CW index", message_type_id::tx_data_request, report);
}

/// Validates the payload property of a Tx_Data.request message, as per SCF-222 v4.0 section 3.4.6.
static bool validate_pdu_payload(const shared_transport_block& buffer, validator_report& report)
{
  if (!buffer.get_buffer().empty()) {
    return true;
  }

  report.append(0, "PDU Payload", message_type_id::tx_data_request);
  return false;
}

error_type<validator_report> ocudu::fapi::validate_tx_data_request(const tx_data_request_message& msg)
{
  validator_report report(msg.sfn, msg.slot);

  // Validate the SFN and slot.
  bool success = true;
  success &= validate_sfn(msg.sfn, message_type_id::tx_data_request, report);
  success &= validate_slot(msg.slot, message_type_id::tx_data_request, report);

  for (const auto& pdu : msg.pdus) {
    success &= validate_pdu_cw_index(pdu.cw_index, report);
    success &= validate_pdu_payload(pdu.pdu, report);
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
