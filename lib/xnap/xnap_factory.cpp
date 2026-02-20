/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/xnap/xnap_factory.h"
#include "xnap_impl.h"

/// Notice this would be the only place were we include concrete class implementation files.

using namespace ocudu;
using namespace ocucp;

std::unique_ptr<xnap_interface> ocudu::ocucp::create_xnap(const xnap_configuration&              xnap_cfg_,
                                                          std::unique_ptr<xnap_message_notifier> tx_notifier_,
                                                          task_executor&                         ctrl_exec_)
{
  auto xnap = std::make_unique<xnap_impl>(xnap_cfg_, std::move(tx_notifier_), ctrl_exec_);
  return xnap;
}
