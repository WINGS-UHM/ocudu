// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "formatter/formatter_helpers.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// RACH indication pdu preamble.
struct rach_indication_pdu_preamble {
  uint8_t                      preamble_index;
  std::optional<phy_time_unit> timing_advance_offset;
  uint32_t                     preamble_pwr;
  uint8_t                      preamble_snr;
};

/// RACH indication pdu.
struct rach_indication_pdu {
  uint32_t                                                                      handle = 0;
  uint8_t                                                                       symbol_index;
  uint8_t                                                                       slot_index;
  uint8_t                                                                       ra_index;
  uint32_t                                                                      avg_rssi;
  uint8_t                                                                       avg_snr;
  static_vector<rach_indication_pdu_preamble, MAX_PREAMBLES_PER_PRACH_OCCASION> preambles;
};

/// RACH indication message
struct rach_indication {
  slot_point          slot;
  rach_indication_pdu pdu;
};

} // namespace fapi
} // namespace ocudu

namespace fmt {
template <>
struct formatter<ocudu::fapi::rach_indication> {
private:
  /// Converts the given FAPI RACH occasion RSSI to dB as per SCF-222 v4.0 section 3.4.11.
  static float to_rach_rssi_dB(int fapi_rssi) { return (fapi_rssi - 140000) * 0.001F; }

  /// Converts the given FAPI RACH occasion SNR to dB as per SCF-222 v4.0 section 3.4.11.
  static float to_rach_snr_dB(int fapi_snr) { return (fapi_snr - 128) * 0.5F; }

  /// Converts the given FAPI RACH preamble power to dB as per SCF-222 v4.0 section 3.4.11.
  static float to_rach_preamble_power_dB(int fapi_power) { return static_cast<float>(fapi_power - 140000) * 0.001F; }

  /// Converts the given FAPI RACH preamble SNR to dB as per SCF-222 v4.0 section 3.4.11.
  static float to_rach_preamble_snr_dB(int fapi_snr) { return (fapi_snr - 128) * 0.5F; }

public:
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::fapi::rach_indication& msg, FormatContext& ctx) const
  {
    format_to(ctx.out(),
              "RACH.indication slot={} symb_idx={} slot_idx={} ra_index={} avg_snr={:.1f} nof_preambles={}",
              msg.slot,
              msg.pdu.symbol_index,
              msg.pdu.slot_index,
              msg.pdu.ra_index,
              to_rach_snr_dB(msg.pdu.avg_snr),
              msg.pdu.preambles.size());

    if (msg.pdu.avg_rssi != std::numeric_limits<decltype(msg.pdu.avg_rssi)>::max()) {
      format_to(ctx.out(), " rssi={:.1f}", to_rach_rssi_dB(msg.pdu.avg_rssi));
    }

    // Log the preambles.
    for (const auto& preamble : msg.pdu.preambles) {
      format_to(ctx.out(), "\n\t\t- PREAMBLE index={}", preamble.preamble_index);

      ocudu::fapi::append_time_advance(ctx, preamble.timing_advance_offset, msg.slot.scs());

      if (preamble.preamble_pwr != std::numeric_limits<decltype(preamble.preamble_pwr)>::max()) {
        format_to(ctx.out(), " pwr={:.1f}", to_rach_preamble_power_dB(preamble.preamble_pwr));
      }
      if (preamble.preamble_snr != std::numeric_limits<decltype(preamble.preamble_snr)>::max()) {
        format_to(ctx.out(), " snr={:.1f}", to_rach_preamble_snr_dB(preamble.preamble_snr));
      }
    }

    return ctx.out();
  }
};
} // namespace fmt
