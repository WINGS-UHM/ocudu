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

#include "ocudu/fapi/p7/messages/uci_pdu_definitions.h"
#include "ocudu/ran/phy_time_unit.h"

namespace ocudu {
namespace fapi {

/// PUSCH UCI PDU information.
struct uci_pusch_pdu {
  uint32_t                     handle = 0U;
  rnti_t                       rnti;
  int16_t                      ul_sinr_metric;
  std::optional<phy_time_unit> timing_advance_offset;
  uint16_t                     rssi;
  uint16_t                     rsrp;
  std::optional<uci_harq_pdu>  harq;
  std::optional<uci_csi_part1> csi_part1;
  std::optional<uci_csi_part2> csi_part2;
};

} // namespace fapi
} // namespace ocudu
