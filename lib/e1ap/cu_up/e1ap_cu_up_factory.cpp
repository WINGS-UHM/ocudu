/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/e1ap/cu_up/e1ap_cu_up_factory.h"
#include "e1ap_cu_up_impl.h"

/// Notice this would be the only place were we include concrete class implementation files.

using namespace ocudu;
using namespace ocuup;

std::unique_ptr<e1ap_interface> ocudu::ocuup::create_e1ap(const e1ap_configuration&    e1ap_cfg_,
                                                          e1_connection_client&        e1_client_handler_,
                                                          e1ap_cu_up_manager_notifier& cu_up_notifier_,
                                                          timer_manager&               timers_,
                                                          task_executor&               cu_up_exec_)
{
  auto e1ap_cu_up =
      std::make_unique<e1ap_cu_up_impl>(e1ap_cfg_, e1_client_handler_, cu_up_notifier_, timers_, cu_up_exec_);
  return e1ap_cu_up;
}
