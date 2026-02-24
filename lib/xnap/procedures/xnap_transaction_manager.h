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

#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/support/async/protocol_transaction_manager.h"

namespace ocudu::ocucp {

class xnap_transaction_manager
{
public:
  xnap_transaction_manager(timer_factory timers) : xn_setup_outcome(timers) {}

  /// XN Setup Response/Failure Event Source.
  protocol_transaction_event_source<asn1::xnap::xn_setup_resp_s, asn1::xnap::xn_setup_fail_s> xn_setup_outcome;
};

} // namespace ocudu::ocucp
