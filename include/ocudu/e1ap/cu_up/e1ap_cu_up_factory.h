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
