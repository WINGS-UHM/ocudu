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
#include "ocudu/asn1/e1ap/e1ap_pdu_contents.h"
#include "ocudu/support/async/event_signal.h"
#include "ocudu/support/async/protocol_transaction_manager.h"

namespace ocudu {
namespace ocucp {

class e1ap_bearer_transaction_manager
{
public:
  e1ap_bearer_transaction_manager(timer_factory timers) :
    context_setup_outcome(timers), context_modification_outcome(timers), context_release_complete(timers)
  {
  }

  void cancel_all()
  {
    context_setup_outcome.stop();
    context_modification_outcome.stop();
    context_release_complete.stop();
  }

  /// E1AP Bearer Context Setup Response/Failure Event Source.
  protocol_transaction_event_source<asn1::e1ap::bearer_context_setup_resp_s, asn1::e1ap::bearer_context_setup_fail_s>
      context_setup_outcome;

  /// E1AP Bearer Context Modification Response/Failure Event Source.
  protocol_transaction_event_source<asn1::e1ap::bearer_context_mod_resp_s, asn1::e1ap::bearer_context_mod_fail_s>
      context_modification_outcome;

  /// E1AP Bearer Context Release Event Source.
  protocol_transaction_event_source<asn1::e1ap::bearer_context_release_complete_s> context_release_complete;
};

} // namespace ocucp
} // namespace ocudu
