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

#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/fapi/p7/messages/uci_pdu_definitions.h"
#include "ocudu/ran/phy_time_unit.h"
#include <bitset>

namespace ocudu {
namespace fapi {

/// SR PDU for format 2, 3, or 4.
struct sr_pdu_format_2_3_4 {
  /// Maximum number of supported bytes in this message.
  static constexpr unsigned MAX_SR_PAYLOAD_SIZE_BITS = 4;

  uint16_t                                 sr_bitlen;
  bounded_bitset<MAX_SR_PAYLOAD_SIZE_BITS> sr_payload;
};

/// UCI PUCCH for format 2, 3, or 4.
struct uci_pucch_pdu_format_2_3_4 {
  static constexpr unsigned BITMAP_SIZE   = 4U;
  static constexpr unsigned SR_BIT        = 0U;
  static constexpr unsigned HARQ_BIT      = 1U;
  static constexpr unsigned CSI_PART1_BIT = 2U;
  static constexpr unsigned CSI_PART2_BIT = 3U;

  enum class format_type : uint8_t { format_2, format_3, format_4 };

  std::bitset<BITMAP_SIZE>     pdu_bitmap;
  uint32_t                     handle;
  uint16_t                     rnti;
  format_type                  pucch_format;
  int16_t                      ul_sinr_metric;
  std::optional<phy_time_unit> timing_advance_offset;
  uint16_t                     rssi;
  uint16_t                     rsrp;
  sr_pdu_format_2_3_4          sr;
  uci_harq_pdu                 harq;
  uci_csi_part1                csi_part1;
  uci_csi_part2                csi_part2;
};

} // namespace fapi
} // namespace ocudu
