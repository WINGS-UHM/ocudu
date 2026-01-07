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

#include "ocudu/f1ap/cu_cp/f1ap_configuration.h"
#include "ocudu/f1ap/cu_cp/f1ap_cu.h"
#include "ocudu/f1ap/f1ap_message_notifier.h"
#include "ocudu/support/executors/task_executor.h"
#include <memory>

namespace ocudu {
namespace ocucp {

/// Creates an instance of an F1AP interface, notifying outgoing packets on the specified listener object.
std::unique_ptr<f1ap_cu> create_f1ap(const f1ap_configuration&   f1ap_cfg_,
                                     f1ap_message_notifier&      f1ap_pdu_notifier_,
                                     f1ap_du_processor_notifier& du_processor_notifier_,
                                     timer_manager&              timers_,
                                     task_executor&              ctrl_exec_);

} // namespace ocucp
} // namespace ocudu
