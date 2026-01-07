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

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/nrppa/nrppa_ue_ids.h"
#include "ocudu/support/format/fmt_to_c_str.h"
#include "ocudu/support/format/prefixed_logger.h"
#include "fmt/format.h"

namespace ocudu::ocucp {

class nrppa_ue_log_prefix
{
public:
  nrppa_ue_log_prefix(ue_index_t ue_index, ran_ue_meas_id_t ran_ue_meas_id, lmf_ue_meas_id_t lmf_ue_meas_id)
  {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer),
                   "ue={} ran_ue={} lmf_ue={}: ",
                   fmt::underlying(ue_index),
                   fmt::underlying(ran_ue_meas_id),
                   fmt::underlying(lmf_ue_meas_id));
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using nrppa_ue_logger = prefixed_logger<nrppa_ue_log_prefix>;

} // namespace ocudu::ocucp

namespace fmt {

// associated formatter
template <>
struct formatter<ocudu::ocucp::nrppa_ue_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocucp::nrppa_ue_log_prefix& o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};
} // namespace fmt
