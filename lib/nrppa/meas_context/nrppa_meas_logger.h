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

#include "ocudu/ran/positioning/positioning_ids.h"
#include "ocudu/support/format/fmt_to_c_str.h"
#include "ocudu/support/format/prefixed_logger.h"
#include "fmt/format.h"

namespace ocudu {
namespace ocucp {

class nrppa_meas_log_prefix
{
public:
  nrppa_meas_log_prefix(ran_meas_id_t ran_meas_id, lmf_meas_id_t lmf_meas_id)
  {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer),
                   "ran_meas_id={} lmf_meas_id={}: ",
                   fmt::underlying(ran_meas_id),
                   fmt::underlying(lmf_meas_id));
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using nrppa_meas_logger = prefixed_logger<nrppa_meas_log_prefix>;

} // namespace ocucp
} // namespace ocudu

namespace fmt {

// associated formatter
template <>
struct formatter<ocudu::ocucp::nrppa_meas_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocucp::nrppa_meas_log_prefix& o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};
} // namespace fmt
