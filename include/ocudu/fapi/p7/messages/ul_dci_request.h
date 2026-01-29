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
#include "ocudu/fapi/p7/messages/dl_pdcch_pdu.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"

namespace ocudu {
namespace fapi {

/// Uplink DCI PDU information.
struct ul_dci_pdu {
  dl_pdcch_pdu pdu;
};

/// Uplink DCI request message.
struct ul_dci_request : public base_message {
  slot_point                                            slot;
  static_vector<ul_dci_pdu, MAX_UL_PDCCH_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
