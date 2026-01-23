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

#include "ocudu/support/format/fmt_to_c_str.h"
#include "ocudu/support/timers.h"
#include "fmt/format.h"

/*
 * This file will hold the interfaces and structures for the
 * F1-U RX metrics collection. This also includes formatting
 * helpers for printing the metrics.
 */
namespace ocudu {
namespace ocuup {

/// This struct will hold relevant metrics for the F1-U RX
struct f1u_rx_metrics_container {
  // TODO: add fields
  unsigned counter;
};

inline std::string format_f1u_rx_metrics(timer_duration metrics_period, const f1u_rx_metrics_container& m)
{
  fmt::memory_buffer buffer;
  // TODO: format fields
  return to_c_str(buffer);
}
} // namespace ocuup
} // namespace ocudu

namespace fmt {
// F1-U RX metrics formatter
template <>
struct formatter<ocudu::ocuup::f1u_rx_metrics_container> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocuup::f1u_rx_metrics_container& m, FormatContext& ctx) const
  {
    // TODO: format fields
    return format_to(ctx.out(), "");
  }
};
} // namespace fmt
