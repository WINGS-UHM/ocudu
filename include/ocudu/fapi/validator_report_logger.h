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

#include "ocudu/fapi/validator_report.h"
#include "ocudu/ocudulog/logger.h"

namespace ocudu {
namespace fapi {

/// Logs the given validator report.
void log_validator_report(const validator_report& report, ocudulog::basic_logger& logger, unsigned sector_id);

} // namespace fapi
} // namespace ocudu
