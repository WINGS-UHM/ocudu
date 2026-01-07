/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "error_notifier_dispatcher.h"
#include "error_notifier_dummy.h"

using namespace ocudu;
using namespace fapi;

static error_notifier_dummy dummy_notifier;

error_notifier_dispatcher::error_notifier_dispatcher() : notifier(&dummy_notifier) {}

void error_notifier_dispatcher::on_error_indication(const error_indication& msg)
{
  notifier->on_error_indication(msg);
}

void error_notifier_dispatcher::set_error_indication_notifier(error_indication_notifier& error_notifier)
{
  notifier = &error_notifier;
}
