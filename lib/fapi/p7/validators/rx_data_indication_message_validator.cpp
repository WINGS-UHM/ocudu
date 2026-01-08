/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
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

/// Validates the PDU value property of the Rx_Data.indication PDU, as per SCF-222 v4.0 section 3.4.7 in table
/// Rx_Data.indication message body.
static bool validate_pdu_value(span<const uint8_t> transport_block, validator_report& report)
{
  if (!transport_block.empty()) {
    return true;
  }

  report.append(0, "PDU tag", message_type_id::rx_data_indication);

  return false;
}

error_type<validator_report> ocudu::fapi::validate_rx_data_indication(const rx_data_indication& msg)
{
  validator_report report(msg.slot);

  static constexpr message_type_id msg_id = message_type_id::rx_data_indication;

  bool success = true;
  // NOTE: Control length property will not be validated.

  for (const auto& pdu : msg.pdus) {
    // NOTE: Handle property will not be validated.
    success &= validate_rnti(to_value(pdu.rnti), msg_id, report);
    success &= validate_harq_id(pdu.harq_id, msg_id, report);
    // NOTE: PDU length property will not be validated.
    // PDUs that were not decoded successfully do not carry data.
    success &= validate_pdu_value(pdu.transport_block, report);
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
