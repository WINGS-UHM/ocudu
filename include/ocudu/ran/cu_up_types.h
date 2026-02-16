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

#include "ocudu/ran/gnb_constants.h"
#include "fmt/format.h"
#include <cstdint>
#include <type_traits>

namespace ocudu {

/// Maximum number of UEs supported by CU-UP (implementation-defined).
enum cu_up_ue_index_t : uint32_t {
  MIN_CU_UP_UE_INDEX     = 0,
  MAX_CU_UP_UE_INDEX     = 1023,
  MAX_NOF_CU_UP_UES      = 1024,
  INVALID_CU_UP_UE_INDEX = MAX_NOF_CU_UP_UES
};

/// Convert integer to CU UE index type.
constexpr cu_up_ue_index_t int_to_ue_index(std::underlying_type_t<cu_up_ue_index_t> idx)
{
  return static_cast<cu_up_ue_index_t>(idx);
}

constexpr std::underlying_type_t<cu_up_ue_index_t> ue_index_to_int(cu_up_ue_index_t idx)
{
  return static_cast<std::underlying_type_t<cu_up_ue_index_t>>(idx);
}

constexpr bool is_cu_up_ue_index_valid(cu_up_ue_index_t ue_idx)
{
  return ue_idx < MAX_NOF_CU_UP_UES;
}

} // namespace ocudu

namespace fmt {

template <>
struct formatter<ocudu::cu_up_ue_index_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(ocudu::cu_up_ue_index_t ue_idx, FormatContext& ctx) const
  {
    if (ocudu::is_cu_up_ue_index_valid(ue_idx)) {
      return format_to(ctx.out(), "{}", underlying(ue_idx));
    }
    return format_to(ctx.out(), "invalid");
  }
};

} // namespace fmt
