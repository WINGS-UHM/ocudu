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

#include "ocudu/ran/cu_types.h"
#include "ocudu/ran/rb_id.h"
#include "ocudu/support/format/prefixed_logger.h"
#include "fmt/format.h"

namespace ocudu {

class sdap_session_trx_log_prefix
{
public:
  sdap_session_trx_log_prefix(uint32_t                     ue_index,
                              pdu_session_id_t             psi,
                              std::optional<qos_flow_id_t> qfi,
                              drb_id_t                     drb_id,
                              const char*                  dir)
  {
    fmt::memory_buffer buffer;
    if (qfi.has_value()) {
      fmt::format_to(std::back_inserter(buffer), "ue={} {} {} {} {}: ", ue_index, psi, qfi.value(), drb_id, dir);
    } else {
      fmt::format_to(std::back_inserter(buffer), "ue={} {} {} {}: ", ue_index, psi, drb_id, dir);
    }
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using sdap_session_trx_logger = prefixed_logger<sdap_session_trx_log_prefix>;

class sdap_session_log_prefix
{
public:
  sdap_session_log_prefix(uint32_t ue_index, pdu_session_id_t psi)
  {
    fmt::memory_buffer buffer;
    fmt::format_to(std::back_inserter(buffer), "ue={} {}: ", ue_index, psi);
    prefix = ocudu::to_c_str(buffer);
  }
  const char* to_c_str() const { return prefix.c_str(); }

private:
  std::string prefix;
};

using sdap_session_logger = prefixed_logger<sdap_session_log_prefix>;

} // namespace ocudu

namespace fmt {

// associated formatters
template <>
struct formatter<ocudu::sdap_session_trx_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(ocudu::sdap_session_trx_log_prefix o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};

// associated formatters
template <>
struct formatter<ocudu::sdap_session_log_prefix> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(ocudu::sdap_session_log_prefix o, FormatContext& ctx) const
  {
    return format_to(ctx.out(), "{}", o.to_c_str());
  }
};

} // namespace fmt
