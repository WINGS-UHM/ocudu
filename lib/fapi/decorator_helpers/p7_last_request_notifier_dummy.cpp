/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "p7_last_request_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;
using namespace fapi;

void p7_last_request_notifier_dummy::on_last_message(slot_point slot)
{
  report_error("Dummy FAPI slot last message notifier cannot handle given last message notification");
}
