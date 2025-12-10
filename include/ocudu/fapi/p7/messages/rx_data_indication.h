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
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_point.h"

namespace ocudu {
namespace fapi {

/// Reception data indication PDU information.
struct rx_data_indication_pdu {
  enum class pdu_tag_type : uint8_t { MAC_PDU, offset, custom = 100 };

  uint32_t     handle;
  rnti_t       rnti;
  uint8_t      rapid;
  uint8_t      harq_id;
  uint32_t     pdu_length;
  pdu_tag_type pdu_tag;
  //: TODO: non-conformant, revise
  const uint8_t* data;
};

/// Reception data indication message.
struct rx_data_indication : public base_message {
  /// Maximum number of supported UCI PDUs in this message.
  static constexpr unsigned MAX_NUM_ULSCH_PDUS_PER_SLOT = 64;

  slot_point                                                         slot;
  uint16_t                                                           control_length;
  static_vector<rx_data_indication_pdu, MAX_NUM_ULSCH_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
