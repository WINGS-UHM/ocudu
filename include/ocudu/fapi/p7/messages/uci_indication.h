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
#include "ocudu/fapi/p7/messages/uci_pucch_pdu_format_0_1.h"
#include "ocudu/fapi/p7/messages/uci_pucch_pdu_format_2_3_4.h"
#include "ocudu/fapi/p7/messages/uci_pusch_pdu.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"
#include <variant>

namespace ocudu {
namespace fapi {

/// UCI indication message.
struct uci_indication : public base_message {
  /// UCI indication PDU format.
  using uci_indication_pdu = std::variant<uci_pusch_pdu, uci_pucch_pdu_format_0_1, uci_pucch_pdu_format_2_3_4>;

  slot_point                                                  slot;
  static_vector<uci_indication_pdu, MAX_UCI_PDUS_PER_UCI_IND> pdus;
};

} // namespace fapi
} // namespace ocudu
