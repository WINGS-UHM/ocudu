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

#include "ocudu/scheduler/config/scheduler_expert_config.h"
#include "ocudu/scheduler/scheduler_configurator.h"

namespace ocudu {

/// \brief Configuration used to create a scheduler instance.
struct scheduler_config {
  const scheduler_expert_config& expert_params;
  sched_configuration_notifier&  config_notifier;
};

} // namespace ocudu
