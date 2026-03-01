// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "p7_last_request_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;
using namespace fapi;

void p7_last_request_notifier_dummy::on_last_message(slot_point slot)
{
  report_error("Dummy FAPI slot last message notifier cannot handle given last message notification");
}
