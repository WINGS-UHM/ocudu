/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "error_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;

void fapi::error_notifier_dummy::on_error_indication(const error_indication& msg)
{
  report_error("Dummy FAPI slot error message notifier cannot handle given error indication");
}
