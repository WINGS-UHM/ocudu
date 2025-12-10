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

#include "ocudu/fapi/common/base_message.h"
#include "ocudu/fapi/common/error_code.h"
#include "ocudu/ran/slot_point.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// Encodes the error indication message.
struct error_indication : public base_message {
  std::optional<slot_point> slot;
  message_type_id           message_id;
  error_code_id             error_code;
  std::optional<slot_point> expected_slot;
};

} // namespace fapi
} // namespace ocudu
