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

#include "ocudu/adt/span.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/fapi/common/base_message.h"
#include "ocudu/ran/harq_id.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"

namespace ocudu {
namespace fapi {

/// Reception data indication PDU information.
struct rx_data_indication_pdu {
  uint32_t            handle = 0;
  rnti_t              rnti;
  harq_id_t           harq_id;
  span<const uint8_t> transport_block;
};

/// Reception data indication message.
struct rx_data_indication : public base_message {
  slot_point                                                     slot;
  static_vector<rx_data_indication_pdu, MAX_PUSCH_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
