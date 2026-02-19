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

#include <cstdint>
#include <type_traits>

namespace ocudu {

/// \c ServCellIndex, as per TS 38.331. It concerns a short identity, used to uniquely identify a serving cell (from
/// a UE's perspective) across cell groups. Value 0 applies to the PCell (Master Cell Group).
enum serv_cell_index_t : uint8_t {
  SERVING_PCELL_IDX     = 0,
  MAX_SERVING_CELL_IDX  = 31,
  MAX_NOF_SCELLS        = 31,
  MAX_NOF_SERVING_CELLS = 32,
  SERVING_CELL_INVALID  = MAX_NOF_SERVING_CELLS
};

constexpr serv_cell_index_t to_serv_cell_index(std::underlying_type_t<serv_cell_index_t> val)
{
  return static_cast<serv_cell_index_t>(val);
}

} // namespace ocudu

namespace fmt {
template <>
struct formatter<ocudu::serv_cell_index_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(ocudu::serv_cell_index_t ue_idx, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", underlying(ue_idx));
  }
};

} // namespace fmt
