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

#include "external/nlohmann/json.hpp"
#include "ocudu/ran/slot_point.h"

namespace ocudu {

/// [Implementation defined] Default indentation size for the JSON metrics.
constexpr unsigned DEFAULT_JSON_INDENT = 2U;

inline void to_json(nlohmann::json& json, slot_point slot)
{
  json = fmt::format("{}", slot);
}

} // namespace ocudu
