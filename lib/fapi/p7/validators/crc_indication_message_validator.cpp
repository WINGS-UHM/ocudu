/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/crc_indication_message_validator.h"
#include "p7_validator_helpers.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/crc_indication.h"

using namespace ocudu;
using namespace fapi;

/// Validates the RSRP property of a CRC.indication PDU, as per SCF-222 v4.0 section 3.4.8.
static bool validate_rsrp(unsigned value, validator_report& report)
{
  static constexpr unsigned INVALID   = 65535;
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1280;

  if (value == INVALID) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "RSRP", message_type_id::crc_indication, report);
}

error_type<validator_report> ocudu::fapi::validate_crc_indication(const crc_indication& msg)
{
  validator_report report(msg.slot);

  bool success = true;

  // Validate each PDU.
  for (const auto& pdu : msg.pdus) {
    // NOTE: Handle property will not be validated as the values are not specified in the document.
    success &= validate_rnti(to_value(pdu.rnti), message_type_id::crc_indication, report);
    success &= validate_harq_id(pdu.harq_id, message_type_id::crc_indication, report);
    // NOTE: CB CRC status bitmap property will not be validated.
    // NOTE: SINR property uses the whole variable range, so it will not be tested.
    success &= validate_timing_advance_offset(pdu.timing_advance_offset, message_type_id::crc_indication, report);
    success &= validate_timing_advance_offset_ns(pdu.timing_advance_offset_ns, message_type_id::crc_indication, report);
    success &= validate_rssi(pdu.rssi, report);
    success &= validate_rsrp(pdu.rsrp, report);
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
