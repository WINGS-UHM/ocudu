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

#include "ocudu/fapi/common/base_message.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/support/shared_transport_block.h"

namespace ocudu {
namespace fapi {

/// Transmission data request PDU information.
struct tx_data_req_pdu {
  tx_data_req_pdu() = default;

  tx_data_req_pdu(uint16_t pdu_index_, uint8_t cw_index_, shared_transport_block pdu_) :
    pdu_index(pdu_index_), cw_index(cw_index_), pdu(std::move(pdu_))
  {
  }

  uint16_t               pdu_index;
  uint8_t                cw_index;
  shared_transport_block pdu;
};

/// Transmission request message.
struct tx_data_request : public base_message {
  slot_point                                              slot;
  static_vector<tx_data_req_pdu, MAX_PDSCH_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
