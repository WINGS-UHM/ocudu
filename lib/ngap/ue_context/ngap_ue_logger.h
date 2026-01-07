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
#include "ocudu/ngap/ngap_types.h"
#include "ocudu/ran/cu_types.h"
#include "ocudu/support/format/fmt_to_c_str.h"
#include "ocudu/support/format/prefixed_logger.h"
#include "fmt/format.h"

namespace ocudu {
namespace ocucp {

class ngap_ue_log_prefix
{
public:
  ngap_ue_log_prefix(ue_index_t  ue_index,
                     ran_ue_id_t ran_ue_id = ran_ue_id_t::invalid,
                     amf_ue_id_t amf_ue_id = amf_ue_id_t::invalid)
  {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer),
                   "ue={}{}{}: ",
                   ue_index,
                   ran_ue_id != ran_ue_id_t::invalid ? fmt::format(" ran_ue={}", fmt::underlying(ran_ue_id)) : "",
                   amf_ue_id != amf_ue_id_t::invalid ? fmt::format(" amf_ue={}", fmt::underlying(amf_ue_id)) : "");
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using ngap_ue_logger = prefixed_logger<ngap_ue_log_prefix>;

} // namespace ocucp
} // namespace ocudu

namespace fmt {

// associated formatter
template <>
struct formatter<ocudu::ocucp::ngap_ue_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(ocudu::ocucp::ngap_ue_log_prefix o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};
} // namespace fmt
