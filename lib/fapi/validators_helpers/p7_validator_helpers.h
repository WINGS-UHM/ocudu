/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "field_checkers.h"
#include "ocudu/fapi/p7/messages/dl_pdu_type.h"
#include "ocudu/fapi/p7/messages/uci_pdu_type.h"
#include "ocudu/fapi/p7/messages/ul_pdu_type.h"
#include "ocudu/scheduler/harq_id.h"

namespace ocudu {
namespace fapi {

/// Validates the number of PDU Groups property of a DL_TTI.request or UL_TTI.request, as per SCF-222 v4.0 section 3.4.2
/// and 3.4.3.
inline bool validate_num_groups(unsigned value, message_type_id msg_type, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 3822;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Number of PDU groups", msg_type, report);
}

/// Validates the RNTI property of a CRC.indication PDU or Rx_Data.indication PDU, as per  SCF-222 v4.0 section 3.4.8
/// and 3.4.7.
inline bool validate_rnti(unsigned value, message_type_id msg_id, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 1;
  static constexpr unsigned MAX_VALUE = 65535;

  return validate_field(MIN_VALUE, MAX_VALUE, static_cast<unsigned>(value), "RNTI", msg_id, report);
}

/// Validates the RAPID property of a CRC.indication PDU or Rx_Data.indication PDU, as per  SCF-222 v4.0 section 3.4.8
/// and 3.4.7.
inline bool validate_rapid(unsigned value, message_type_id msg_id, validator_report& report)
{
  static constexpr unsigned OTHERWISE = 255;
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 63;

  if (value == OTHERWISE) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "RAPID", msg_id, report);
}

/// Validates the HARQ ID property of a CRC.indication PDU or Rx_Data.indication PDU, as per SCF-222 v4.0 section 3.4.8
/// and 3.4.7.
inline bool validate_harq_id(unsigned value, message_type_id msg_id, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = MAX_HARQ_ID;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "HARQ ID", msg_id, report);
}

/// Validates the timing advance offset property of a CRC.indication or SRS.indication PDU, as per SCF-222 v4.0
/// section 3.4.8 and 3.4.10.
inline bool validate_timing_advance_offset(unsigned value, message_type_id msg_id, validator_report& report)
{
  static constexpr unsigned INVALID   = 65535;
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 63;

  if (value == INVALID) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Timing advance offset", msg_id, report);
}

/// Validates the timing advance offset in nanoseconds property of a CRC.indication or SRS.indication PDU, as per
/// SCF-222 v4.0 section 3.4.8 and 3.4.10.
inline bool validate_timing_advance_offset_ns(int value, message_type_id msg_id, validator_report& report)
{
  static constexpr int INVALID   = -32768;
  static constexpr int MIN_VALUE = -16800;
  static constexpr int MAX_VALUE = 16800;

  if (value == INVALID) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "Timing advance offset in nanoseconds", msg_id, report);
}

/// Validates the timing advance offset in nanoseconds property of the RACH.indication PDU, as per SCF-222 v4.0
/// section 3.4.11 in table RACH.indication message body.
inline bool validate_timing_advance_offset_ns(unsigned value, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 2005000;

  if (value == std::numeric_limits<uint32_t>::max()) {
    return true;
  }

  return validate_field(
      MIN_VALUE, MAX_VALUE, value, "Timing advance offset in nanoseconds", message_type_id::rach_indication, report);
}

/// Validates the RSSI property of a CRC.indication PDU, as per SCF-222 v4.0 section 3.4.8.
inline bool validate_rssi(unsigned value, validator_report& report)
{
  static constexpr unsigned INVALID   = 65535;
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1280;

  if (value == INVALID) {
    return true;
  }

  return validate_field(MIN_VALUE, MAX_VALUE, value, "RSSI", message_type_id::crc_indication, report);
}

/// Returns a string identifier for the given message and PDU.
inline const char* get_pdu_type_string(message_type_id msg_id, unsigned pdu_id)
{
  switch (msg_id) {
    case message_type_id::rach_indication:
    case message_type_id::crc_indication:
    case message_type_id::error_indication:
    case message_type_id::tx_data_request:
    case message_type_id::ul_dci_request:
    case message_type_id::rx_data_indication:
    case message_type_id::slot_indication:
    case message_type_id::config_request:
    case message_type_id::config_response:
    case message_type_id::dl_tti_response:
    case message_type_id::param_request:
    case message_type_id::param_response:
    case message_type_id::srs_indication:
    case message_type_id::start_request:
    case message_type_id::stop_indication:
    case message_type_id::stop_request:
      return "";
    case message_type_id::uci_indication:
      return get_uci_pdu_type_string(static_cast<uci_pdu_type>(pdu_id));
    case message_type_id::dl_tti_request:
      return get_dl_tti_pdu_type_string(static_cast<dl_pdu_type>(pdu_id));
    case message_type_id::ul_tti_request:
      return get_ul_tti_pdu_type_string(static_cast<ul_pdu_type>(pdu_id));
    default:
      ocudu_assert(0, "Invalid FAPI message type={}", fmt::underlying(msg_id));
      break;
  }
  return "";
}

inline void log_pdu_and_range_report(fmt::memory_buffer& buffer, const validator_report::error_report& report)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\t- PDU type={}, property={}, value={}, expected value=[{}-{}]\n",
                 get_pdu_type_string(report.message_type, report.pdu_type.value()),
                 report.property_name,
                 report.value,
                 report.expected_value_range.value().first,
                 report.expected_value_range.value().second);
}

inline void log_pdu_report(fmt::memory_buffer& buffer, const validator_report::error_report& report)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\t- PDU type={}, property={}, value={}\n",
                 get_pdu_type_string(report.message_type, report.pdu_type.value()),
                 report.property_name,
                 report.value);
}

} // namespace fapi
} // namespace ocudu
