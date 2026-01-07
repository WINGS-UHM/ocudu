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
#include "ocudu/adt/span.h"
#include "ocudu/ran/pci.h"
#include <string>

namespace ocudu {

namespace odu {
struct o_du_low_metrics;
}

namespace app_helpers {
namespace json_generators {

/// Generates a nlohmann JSON object that codifies the given O-DU low metrics.
nlohmann::json generate(const odu::o_du_low_metrics& metrics);

/// Generates a string in JSON format that codifies the given O-DU low metrics.
std::string generate_string(const odu::o_du_low_metrics& metrics, int indent = -1);

} // namespace json_generators
} // namespace app_helpers
} // namespace ocudu
