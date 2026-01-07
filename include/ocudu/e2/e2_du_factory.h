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

#include "ocudu/du/du_high/du_manager/du_configurator.h"
#include "ocudu/e2/e2_du.h"
#include "ocudu/e2/e2ap_configuration.h"
#include "ocudu/e2/gateways/e2_connection_client.h"
#include "ocudu/f1ap/du/f1ap_du.h"
#include "ocudu/support/timers.h"

namespace ocudu {

/// Creates a instance of an E2 interface (with subscription manager)
std::unique_ptr<e2_agent> create_e2_du_agent(const e2ap_configuration&   e2ap_cfg_,
                                             e2_connection_client&       e2_client_,
                                             e2_du_metrics_interface*    e2_metrics_var,
                                             odu::f1ap_ue_id_translator* f1ap_ue_id_translator_,
                                             odu::du_configurator*       du_configurator_,
                                             timer_factory               timers_,
                                             task_executor&              e2_exec_);

} // namespace ocudu
