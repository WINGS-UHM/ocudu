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

#include "scheduler_policy.h"
#include "ocudu/scheduler/config/scheduler_expert_config.h"
#include <memory>

namespace ocudu {

class cell_configuration;

/// Creatre intra-cell, intra-slice scheduler policy.
std::unique_ptr<scheduler_policy> create_scheduler_strategy(const scheduler_policy_config& policy_cfg,
                                                            const cell_configuration&      cell_cfg);

} // namespace ocudu
