// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "formatter/formatter_helpers.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/harq_id.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// Reception data indication PDU information.
struct crc_ind_pdu {
  uint32_t                     handle = 0;
  rnti_t                       rnti;
  harq_id_t                    harq_id;
  bool                         tb_crc_status_ok;
  int16_t                      ul_sinr_metric;
  std::optional<phy_time_unit> timing_advance_offset;
  uint16_t                     rssi;
  uint16_t                     rsrp;
};

/// CRC indication message.
struct crc_indication {
  slot_point                                          slot;
  static_vector<crc_ind_pdu, MAX_PUSCH_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu

namespace fmt {
template <>
struct formatter<ocudu::fapi::crc_indication> {
private:
  /// Converts the given FAPI CRC SINR to dB as per SCF-222 v4.0 section 3.4.8.
  static float to_crc_ul_sinr(int sinr) { return static_cast<float>(sinr) * 0.002F; }

  /// Converts the given FAPI CRC RSSI to dB as per SCF-222 v4.0 section 3.4.8.
  static float to_crc_ul_rssi(unsigned rssi) { return static_cast<float>(static_cast<int>(rssi) - 1280) * 0.1F; }

  /// Converts the given FAPI CRC RSRP to dB as per SCF-222 v4.0 section 3.4.8.
  static float to_crc_ul_rsrp(unsigned rsrp) { return static_cast<float>(static_cast<int>(rsrp) - 1280) * 0.1F; }

public:
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::fapi::crc_indication& msg, FormatContext& ctx) const
  {
    format_to(ctx.out(), "CRC.indication slot={}", msg.slot);

    for (const auto& pdu : msg.pdus) {
      format_to(ctx.out(),
                "\n\t- CRC rnti={} harq_id={} tb_status={}",
                pdu.rnti,
                underlying(pdu.harq_id),
                pdu.tb_crc_status_ok ? "OK" : "KO");

      ocudu::fapi::append_time_advance(ctx, pdu.timing_advance_offset, msg.slot.scs());

      if (pdu.ul_sinr_metric != std::numeric_limits<decltype(pdu.ul_sinr_metric)>::min()) {
        format_to(ctx.out(), " sinr={:.1f}", to_crc_ul_sinr(pdu.ul_sinr_metric));
      }
      if (pdu.rssi != std::numeric_limits<decltype(pdu.rssi)>::max()) {
        format_to(ctx.out(), " rssi={:.1f}", to_crc_ul_rssi(pdu.rssi));
      }
      if (pdu.rsrp != std::numeric_limits<decltype(pdu.rsrp)>::max()) {
        format_to(ctx.out(), " rsrp={:.1f}", to_crc_ul_rsrp(pdu.rsrp));
      }
    }

    return ctx.out();
  }
};
} // namespace fmt
