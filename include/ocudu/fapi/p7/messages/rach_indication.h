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
#include "ocudu/ran/phy_time_unit.h"
#include "ocudu/ran/rnti.h"
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
struct rach_indication : public base_message {
  slot_point                                                       slot;
  static_vector<rach_indication_pdu, MAX_PRACH_OCCASIONS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
