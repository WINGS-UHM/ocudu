/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/ran/uci/uci_constants.h"
#include "ocudu/ran/uci/uci_mapping.h"

namespace ocudu {
namespace fapi {

/// UCI CSI Part 1 information.
struct uci_csi_part1 {
  uci_pusch_or_pucch_f2_3_4_detection_status                     detection_status;
  uint16_t                                                       expected_bit_length;
  bounded_bitset<uci_constants::MAX_NOF_CSI_PART1_OR_PART2_BITS> payload;
};

/// UCI CSI Part 2 information.
struct uci_csi_part2 {
  uci_pusch_or_pucch_f2_3_4_detection_status                     detection_status;
  uint16_t                                                       expected_bit_length;
  bounded_bitset<uci_constants::MAX_NOF_CSI_PART1_OR_PART2_BITS> payload;
};

/// UCI HARQ PDU information.
struct uci_harq_pdu {
  uci_pusch_or_pucch_f2_3_4_detection_status       detection_status;
  uint16_t                                         expected_bit_length;
  bounded_bitset<uci_constants::MAX_NOF_HARQ_BITS> payload;
};

} // namespace fapi
} // namespace ocudu
