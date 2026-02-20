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

#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_configuration.h"
#include <memory>

namespace ocudu::ocucp {

/// Creates an instance of an XNAP interface, notifying outgoing packets on the specified listener object.
std::unique_ptr<xnap_interface> create_xnap(const xnap_configuration&              xnap_cfg_,
                                            std::unique_ptr<xnap_message_notifier> tx_notifier_,
                                            task_executor&                         ctrl_exec_);

} // namespace ocudu::ocucp
