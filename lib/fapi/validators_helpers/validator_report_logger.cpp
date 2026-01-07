/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/validator_report_logger.h"
#include "p7_validator_helpers.h"
#include "validator_helpers.h"
#include "ocudu/support/format/fmt_to_c_str.h"

using namespace ocudu;
using namespace fapi;

/// Logs the given validator report.
void ocudu::fapi::log_validator_report(const validator_report& report,
                                       ocudulog::basic_logger& logger,
                                       unsigned                sector_id)
{
  fmt::memory_buffer str_buffer;
  fmt::format_to(std::back_inserter(str_buffer),
                 "Sector#{}: Detected {} error(s) in message type '{}' in slot={}.{}:\n",
                 sector_id,
                 report.reports.size(),
                 get_message_type_string(report.reports.front().message_type),
                 report.sfn,
                 report.slot);

  for (const auto& error : report.reports) {
    // Basic + PDU + Range log.
    if (error.pdu_type.has_value() && error.expected_value_range.has_value()) {
      log_pdu_and_range_report(str_buffer, error);
      continue;
    }

    // Basic + PDU log.
    if (error.pdu_type.has_value()) {
      log_pdu_report(str_buffer, error);
      continue;
    }

    // Basic + Range log.
    if (error.expected_value_range.has_value()) {
      log_range_report(str_buffer, error);
      continue;
    }

    // Basic log.
    log_basic_report(str_buffer, error);
  }

  logger.warning("{}", to_c_str(str_buffer));
}
