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
#include "ocudu/support/timers.h"

namespace ocudu {
namespace ocucp {

using e1ap_outcome     = expected<asn1::e1ap::successful_outcome_s, asn1::e1ap::unsuccessful_outcome_s>;
using e1ap_transaction = protocol_transaction<e1ap_outcome>;

class e1ap_transaction_manager
{
public:
  /// Transaction Response Container, which gets indexed by transaction_id.
  static constexpr size_t                    MAX_NOF_TRANSACTIONS = 256;
  protocol_transaction_manager<e1ap_outcome> transactions;

  explicit e1ap_transaction_manager(timer_factory timers) : transactions(MAX_NOF_TRANSACTIONS, timers) {}
};

} // namespace ocucp
} // namespace ocudu
