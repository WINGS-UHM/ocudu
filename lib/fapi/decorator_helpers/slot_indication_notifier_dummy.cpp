/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "slot_indication_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;
using namespace fapi;

void slot_indication_notifier_dummy::on_slot_indication(const slot_indication& msg)
{
  report_error("Dummy FAPI slot indication notifier cannot handle given slot indication");
}
