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

#include "ocudu/ran/uci/uci_payload_type.h"
#include "ocudu/support/units.h"

namespace ocudu {
namespace fapi {

/// UCI CSI Part 1 information.
struct uci_csi_part1 {
  uci_pusch_or_pucch_f2_3_4_detection_status detection_status;
  units::bits                                expected_bit_length;
  uci_payload_type                           payload;
};

/// UCI CSI Part 2 information.
struct uci_csi_part2 {
  uci_pusch_or_pucch_f2_3_4_detection_status detection_status;
  units::bits                                expected_bit_length;
  uci_payload_type                           payload;
};

/// UCI HARQ PDU information.
struct uci_harq_pdu {
  uci_pusch_or_pucch_f2_3_4_detection_status detection_status;
  units::bits                                expected_bit_length;
  uci_payload_type                           payload;
};

} // namespace fapi
} // namespace ocudu
