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

struct error_indication_message;

/// Validates the given ERROR.indication message and returns a report for the result of the validation. The validation
/// checks every property of the message, as per SCF-222 v4.0 Section 3.3.6.1.
error_type<validator_report> validate_error_indication(const error_indication_message& msg);

} // namespace fapi
} // namespace ocudu
