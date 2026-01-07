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

#include "ocudu/fapi/p7/messages/uci_pdu_type.h"
#include "ocudu/fapi/p7/messages/uci_pucch_pdu_format_0_1.h"
#include "ocudu/fapi/p7/messages/uci_pucch_pdu_format_2_3_4.h"
#include "ocudu/fapi/p7/messages/uci_pusch_pdu.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"

namespace ocudu {
namespace fapi {

/// Reception data indication PDU information.
struct uci_indication_pdu {
  uci_pdu_type pdu_type;
  uint16_t     pdu_size;

  // :TODO: add a variant for this fields below.
  uci_pusch_pdu              pusch_pdu;
  uci_pucch_pdu_format_0_1   pucch_pdu_f01;
  uci_pucch_pdu_format_2_3_4 pucch_pdu_f234;
};

/// UCI indication message.
struct uci_indication : public base_message {
  uint16_t                                                    sfn;
  uint16_t                                                    slot;
  static_vector<uci_indication_pdu, MAX_UCI_PDUS_PER_UCI_IND> pdus;
};

} // namespace fapi
} // namespace ocudu
