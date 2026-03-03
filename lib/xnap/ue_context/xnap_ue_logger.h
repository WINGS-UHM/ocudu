// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/support/format/fmt_to_c_str.h"
#include "ocudu/support/format/prefixed_logger.h"
#include "ocudu/xnap/xnap_types.h"
#include "fmt/format.h"

namespace ocudu::ocucp {

class xnap_ue_log_prefix
{
public:
  xnap_ue_log_prefix(ue_index_t ue_index, xnap_ue_id_t xnap_ue_id = xnap_ue_id_t::invalid)
  {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer),
                   "ue={}{}: ",
                   ue_index,
                   xnap_ue_id != xnap_ue_id_t::invalid ? fmt::format(" xnap_ue={}", fmt::underlying(xnap_ue_id)) : "");
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using xnap_ue_logger = prefixed_logger<xnap_ue_log_prefix>;

} // namespace ocudu::ocucp

namespace fmt {

// associated formatter
template <>
struct formatter<ocudu::ocucp::xnap_ue_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocucp::xnap_ue_log_prefix& o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};
} // namespace fmt
