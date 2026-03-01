// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "error_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;

void fapi::error_notifier_dummy::on_error_indication(const error_indication& msg)
{
  report_error("Dummy FAPI slot error message notifier cannot handle given error indication");
}
