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

#include "ocudu/ran/gnb_du_id.h"
#include "ocudu/ran/rb_id.h"
#include "ocudu/support/format/fmt_to_c_str.h"
#include "ocudu/support/format/prefixed_logger.h"
#include "fmt/format.h"

namespace ocudu {

class rlc_bearer_log_prefix
{
public:
  rlc_bearer_log_prefix(gnb_du_id_t gnb_du_id, uint32_t ue_index, rb_id_t rb_id, const char* dir)
  {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "du={} ue={} {} {}: ", fmt::underlying(gnb_du_id), ue_index, rb_id, dir);
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using rlc_bearer_logger = prefixed_logger<rlc_bearer_log_prefix>;

} // namespace ocudu

namespace fmt {

// associated formatter
template <>
struct formatter<ocudu::rlc_bearer_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::rlc_bearer_log_prefix& o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};
} // namespace fmt
