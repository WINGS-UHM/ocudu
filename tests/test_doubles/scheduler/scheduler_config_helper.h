// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/scheduler/config/cell_config_builder_params.h"
#include "ocudu/scheduler/config/ran_cell_config.h"
#include "ocudu/scheduler/scheduler_configurator.h"

namespace ocudu::sched_config_helper {

/// Create a default cell configuration request.
sched_cell_configuration_request_message
make_default_sched_cell_configuration_request(const cell_config_builder_params& params = {});

/// Create a default UE creation request.
sched_ue_creation_request_message create_default_sched_ue_creation_request(const ran_cell_config& cell_cfg,
                                                                           span<const lcid_t>     lcid_to_cfg = {});
sched_ue_creation_request_message
create_default_sched_ue_creation_request(const ran_cell_config&               cell_cfg,
                                         const std::initializer_list<lcid_t>& lcid_to_cfg);

/// Create a UE creation request with no UE-dedicated config.
sched_ue_creation_request_message create_empty_spcell_cfg_sched_ue_creation_request(const ran_cell_config& cell_cfg);

} // namespace ocudu::sched_config_helper
