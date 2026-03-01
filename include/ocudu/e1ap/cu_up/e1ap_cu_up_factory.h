// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/e1ap/cu_up/e1ap_configuration.h"
#include "ocudu/e1ap/cu_up/e1ap_cu_up.h"
#include "ocudu/e1ap/gateways/e1_connection_client.h"
#include "ocudu/support/executors/task_executor.h"
#include "ocudu/support/timers.h"
#include <memory>

namespace ocudu {
namespace ocuup {

/// Creates an instance of an E1AP interface, notifying outgoing packets on the specified listener object.
std::unique_ptr<e1ap_interface> create_e1ap(const e1ap_configuration&    e1ap_cfg_,
                                            e1_connection_client&        e1_client_handler_,
                                            e1ap_cu_up_manager_notifier& cu_up_notifier_,
                                            timer_manager&               timers_,
                                            task_executor&               cu_up_exec_);

} // namespace ocuup
} // namespace ocudu
