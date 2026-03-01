// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

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
