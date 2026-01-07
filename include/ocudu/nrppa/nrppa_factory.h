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

#include "ocudu/cu_cp/common_task_scheduler.h"
#include "ocudu/cu_cp/cu_cp_configuration.h"
#include "ocudu/nrppa/nrppa.h"

namespace ocudu {
namespace ocucp {

/// Creates an instance of an NRPPA interface, notifying outgoing packets on the specified listener object.
std::unique_ptr<nrppa_interface> create_nrppa(const cu_cp_configuration& cfg,
                                              nrppa_cu_cp_notifier&      cu_cp_notifier,
                                              common_task_scheduler&     common_task_sched);

} // namespace ocucp
} // namespace ocudu
