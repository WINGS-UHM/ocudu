// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/du/du_high/du_test_mode_config.h"
#include "ocudu/f1ap/du/f1ap_du.h"
#include <memory>

namespace ocudu {
namespace odu {

class f1ap_ue_executor_mapper;

/// Creates an F1AP handler for the DU-high.
std::unique_ptr<f1ap_du> create_du_high_f1ap(f1c_connection_client&     f1c_client_handler,
                                             f1ap_du_configurator&      du_mng,
                                             task_executor&             ctrl_exec,
                                             f1ap_ue_executor_mapper&   ue_exec_mapper,
                                             f1ap_du_paging_notifier&   paging_notifier,
                                             timer_manager&             timers,
                                             const du_test_mode_config& test_cfg);

} // namespace odu
} // namespace ocudu
