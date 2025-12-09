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

#include "ocudu/adt/expected.h"
#include "ocudu/fapi/validator_report.h"

namespace ocudu {
namespace fapi {

struct dl_tti_request_message;

/// Validates the given DL_TTI.request message and returns a report for the result of the validation. The validation
/// checks every property of the message, as per SCF-222 v4.0 Section 3.4.2.
error_type<validator_report> validate_dl_tti_request(const dl_tti_request_message& msg);

} // namespace fapi
} // namespace ocudu
