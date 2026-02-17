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

#include "ocudu/scheduler/scheduler_configurator.h"

namespace ocudu {
namespace odu {

struct du_cell_config;
struct du_ue_context;
struct du_ue_resource_config;

/// Derive Scheduler SI Scheduling Configuration from DU Cell Configuration.
si_scheduling_config make_si_scheduling_info_config(const du_cell_config&    du_cfg,
                                                    units::bytes             sib1_len,
                                                    span<const units::bytes> si_message_lens);

/// Derives Scheduler Cell Configuration from DU Cell Configuration.
sched_cell_configuration_request_message make_sched_cell_config_req(du_cell_index_t             cell_index,
                                                                    const odu::du_cell_config&  du_cfg,
                                                                    const si_scheduling_config& si_sched_cfg);

/// Create scheduler UE Configuration Request based on DU UE configuration context.
sched_ue_config_request create_scheduler_ue_config_request(const du_ue_context&         ue_ctx,
                                                           const du_ue_resource_config& ue_res_cfg);

} // namespace odu
} // namespace ocudu
