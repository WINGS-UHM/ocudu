/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "field_checkers.h"
#include "ocudu/ran/harq_id.h"

namespace ocudu {
namespace fapi {

/// Validates the SFN property of a message.
inline bool validate_sfn(unsigned value, message_type_id msg_type, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 1023;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "sfn", msg_type, report);
}

/// Validates the slot property of a message.
inline bool validate_slot(unsigned value, message_type_id msg_type, validator_report& report)
{
  static constexpr unsigned MIN_VALUE = 0;
  static constexpr unsigned MAX_VALUE = 159;

  return validate_field(MIN_VALUE, MAX_VALUE, value, "slot", msg_type, report);
}

inline void log_range_report(fmt::memory_buffer& buffer, const validator_report::error_report& report)
{
  fmt::format_to(std::back_inserter(buffer),
                 "\t- Property={}, value={}, expected value=[{}-{}]\n",
                 report.property_name,
                 report.value,
                 report.expected_value_range.value().first,
                 report.expected_value_range.value().second);
}

inline void log_basic_report(fmt::memory_buffer& buffer, const validator_report::error_report& report)
{
  fmt::format_to(std::back_inserter(buffer), "\t- Property={}, value={}\n", report.property_name, report.value);
}

} // namespace fapi
} // namespace ocudu
