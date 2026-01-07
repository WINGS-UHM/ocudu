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

#include "ocudu/adt/static_vector.h"

namespace ocudu {
namespace fapi {

/// UCI information for determining UCI Part1 to Part2 correspondence.
struct uci_part1_to_part2_correspondence_v3 {
  /// Maximum number of part2 info.
  static constexpr unsigned MAX_NUM_PART2_INFO = 100;

  enum class map_scope_type : uint8_t { common_context, phy_context };

  struct part2_info {
    uint16_t                   priority;
    static_vector<uint16_t, 4> param_offsets;
    static_vector<uint8_t, 4>  param_sizes;
    uint16_t                   part2_size_map_index;
    map_scope_type             part2_size_map_scope;
  };

  static_vector<part2_info, MAX_NUM_PART2_INFO> part2;
};

} // namespace fapi
} // namespace ocudu
