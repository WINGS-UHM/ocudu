// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/pusch/pusch_context.h"

namespace fmt {

/// \brief Custom formatter for \c pusch_context.
template <>
struct formatter<ocudu::pusch_context> {
public:
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return helper.parse(ctx);
  }

  template <typename FormatContext>
  auto format(const ocudu::pusch_context& context, FormatContext& ctx) const
  {
    helper.format_always(ctx, "rnti={}", context.rnti);
    helper.format_always(ctx, "h_id={}", fmt::underlying(context.h_id));
    return ctx.out();
  }

private:
  ocudu::delimited_formatter helper;
};

} // namespace fmt
