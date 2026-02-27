// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "formatter/formatter_helpers.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/ran/srs/srs_channel_matrix.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// Encodes SRS positioning report.
struct srs_positioning_report {
  /// TUL-RTOA as defined in TS 38.215 on section 5.1.
  std::optional<phy_time_unit> ul_relative_toa;
  std::optional<float>         rsrp;
};

/// SRS indication pdu.
struct srs_indication_pdu {
  uint32_t                              handle = 0;
  rnti_t                                rnti;
  std::optional<phy_time_unit>          timing_advance_offset;
  std::optional<srs_channel_matrix>     matrix;
  std::optional<srs_positioning_report> positioning;
};

/// SRS indication message.
struct srs_indication {
  slot_point                                                  slot;
  static_vector<srs_indication_pdu, MAX_SRS_PDUS_PER_SRS_IND> pdus;
};

} // namespace fapi
} // namespace ocudu

namespace fmt {
template <>
struct formatter<ocudu::fapi::srs_indication> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::fapi::srs_indication& msg, FormatContext& ctx) const
  {
    format_to(ctx.out(), "SRS.indication slot={}", msg.slot);

    for (const auto& pdu : msg.pdus) {
      format_to(ctx.out(), "\n\t-  rnti={}", pdu.rnti);

      ocudu::fapi::append_time_advance(ctx, pdu.timing_advance_offset, msg.slot.scs());

      if (!pdu.positioning.has_value()) {
        continue;
      }

      if (pdu.positioning->ul_relative_toa) {
        format_to(ctx.out(), " RTOA_s={}", *pdu.positioning->rsrp);
      }
      if (pdu.positioning->rsrp) {
        format_to(ctx.out(), " RSRP={}", *pdu.positioning->rsrp);
      }
    }

    return ctx.out();
  }
};
} // namespace fmt
