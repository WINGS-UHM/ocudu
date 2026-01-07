/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_processor_factory.h"
#include "du_processor_impl.h"
#include "ocudu/cu_cp/common_task_scheduler.h"

/// Notice this would be the only place were we include concrete class implementation files.

using namespace ocudu;
using namespace ocucp;

std::unique_ptr<du_processor> ocudu::ocucp::create_du_processor(du_processor_config_t        du_processor_config,
                                                                du_processor_cu_cp_notifier& cu_cp_notifier_,
                                                                f1ap_message_notifier&       f1ap_pdu_notifier_,
                                                                common_task_scheduler&       common_task_sched_,
                                                                ue_manager&                  ue_mng_)
{
  auto du_processor = std::make_unique<du_processor_impl>(
      std::move(du_processor_config), cu_cp_notifier_, f1ap_pdu_notifier_, common_task_sched_, ue_mng_);
  return du_processor;
}
