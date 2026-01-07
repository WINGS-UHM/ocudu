/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "p7_last_request_notifier_dispatcher.h"
#include "p7_last_request_notifier_dummy.h"

using namespace ocudu;
using namespace fapi;

static p7_last_request_notifier_dummy dummy_notifier;

p7_last_request_notifier_dispatcher::p7_last_request_notifier_dispatcher() : p7_notifier(&dummy_notifier) {}

void p7_last_request_notifier_dispatcher::on_last_message(slot_point slot)
{
  p7_notifier->on_last_message(slot);
}

void p7_last_request_notifier_dispatcher::set_p7_last_request_notifier(p7_last_request_notifier& p7_last_req_notifier)
{
  p7_notifier = &p7_last_req_notifier;
}
