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

#include "ocudu/adt/expected.h"
#include "ocudu/asn1/e1ap/e1ap.h"
#include "ocudu/support/async/event_signal.h"
#include "ocudu/support/async/protocol_transaction_manager.h"

namespace ocudu {
namespace ocuup {

/// Response type of a E1AP CU-UP transaction.
using e1ap_transaction_response = expected<asn1::e1ap::successful_outcome_s, asn1::e1ap::unsuccessful_outcome_s>;

/// E1AP protocol transaction type.
using e1ap_transaction = protocol_transaction<e1ap_transaction_response>;

class e1ap_event_manager
{
  /// Transaction Response Container, which gets indexed by transaction_id.
  static constexpr size_t MAX_NOF_TRANSACTIONS = 256;

public:
  protocol_transaction_manager<e1ap_transaction_response> transactions;

  explicit e1ap_event_manager(timer_factory timer_service) : transactions(MAX_NOF_TRANSACTIONS, timer_service) {}
};

} // namespace ocuup
} // namespace ocudu
