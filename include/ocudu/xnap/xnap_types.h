// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "fmt/base.h"
#include <cstdint>
#include <type_traits>

namespace ocudu::ocucp {

/// \brief NG-RAN node UE XnAP ID (non ASN1 type of XNAP_UE_ID) used to identify the UE in the XNAP.
/// \remark See TS 38.423 Section 9.2.3.16: NG-RAN node UE XnAP ID valid values: (0..2^32-1)
constexpr uint64_t MAX_NOF_XNAP_UES = ((uint64_t)1 << 32);
enum class xnap_ue_id_t : uint64_t { min = 0, max = MAX_NOF_XNAP_UES - 1, invalid = 0x1fffffff };

/// Convert XNAP_UE_ID type to integer.
constexpr uint64_t xnap_ue_id_to_uint(xnap_ue_id_t id)
{
  return static_cast<uint64_t>(id);
}

/// Convert integer to XNAP_UE_ID type.
constexpr xnap_ue_id_t uint_to_xnap_ue_id(std::underlying_type_t<xnap_ue_id_t> id)
{
  return static_cast<xnap_ue_id_t>(id);
}

} // namespace ocudu::ocucp

// XNAP UE ID formatter.
template <>
struct fmt::formatter<ocudu::ocucp::xnap_ue_id_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocucp::xnap_ue_id_t& idx, FormatContext& ctx) const
  {
    if (idx == ocudu::ocucp::xnap_ue_id_t::invalid) {
      return format_to(ctx.out(), "invalid");
    }
    return format_to(ctx.out(), "{}", (unsigned)idx);
  }
};
