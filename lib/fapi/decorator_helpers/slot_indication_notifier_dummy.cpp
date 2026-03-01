// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "slot_indication_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;
using namespace fapi;

void slot_indication_notifier_dummy::on_slot_indication(const slot_indication& msg)
{
  report_error("Dummy FAPI slot indication notifier cannot handle given slot indication");
}
