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

#include "ocudu/adt/static_vector.h"
#include "ocudu/fapi/common/base_message.h"
#include "ocudu/fapi/p7/messages/uci_pdu_definitions.h"
#include "ocudu/ran/phy_time_unit.h"
#include <bitset>

namespace ocudu {
namespace fapi {

/// PUSCH UCI PDU information.
struct uci_pusch_pdu {
  static constexpr unsigned BITMAP_SIZE   = 4U;
  static constexpr unsigned HARQ_BIT      = 1U;
  static constexpr unsigned CSI_PART1_BIT = 2U;
  static constexpr unsigned CSI_PART2_BIT = 3U;

  std::bitset<BITMAP_SIZE>     pdu_bitmap;
  uint32_t                     handle;
  uint16_t                     rnti;
  int16_t                      ul_sinr_metric;
  std::optional<phy_time_unit> timing_advance_offset;
  uint16_t                     rssi;
  uint16_t                     rsrp;
  uci_harq_pdu                 harq;
  uci_csi_part1                csi_part1;
  uci_csi_part2                csi_part2;
};

} // namespace fapi
} // namespace ocudu
