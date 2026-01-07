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

#include "ocudu/ngap/ngap.h"
#include "ocudu/ngap/ngap_configuration.h"
#include "ocudu/support/executors/task_executor.h"
#include <memory>

namespace ocudu {

namespace ocucp {

class n2_connection_client;

/// Creates an instance of an NGAP interface, notifying outgoing packets on the specified listener object.
std::unique_ptr<ngap_interface> create_ngap(const ngap_configuration& ngap_cfg_,
                                            ngap_cu_cp_notifier&      cu_cp_notifier_,
                                            n2_connection_client&     n2_gateway_handler_,
                                            timer_manager&            timers_,
                                            task_executor&            ctrl_exec_);

} // namespace ocucp

} // namespace ocudu
