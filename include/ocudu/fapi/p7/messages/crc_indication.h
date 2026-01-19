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
#include "ocudu/ran/harq_id.h"
#include "ocudu/ran/phy_time_unit.h"
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
struct crc_indication : public base_message {
  slot_point                                          slot;
  static_vector<crc_ind_pdu, MAX_PUSCH_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
