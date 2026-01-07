/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "scheduler_policy_factory.h"
#include "scheduler_time_qos.h"
#include "scheduler_time_rr.h"

using namespace ocudu;

std::unique_ptr<scheduler_policy> ocudu::create_scheduler_strategy(const scheduler_policy_config& policy_cfg,
                                                                   const cell_configuration&      cell_cfg)
{
  if (std::holds_alternative<time_rr_scheduler_config>(policy_cfg)) {
    return std::make_unique<scheduler_time_rr>();
  }
  if (std::holds_alternative<time_qos_scheduler_config>(policy_cfg)) {
    return std::make_unique<scheduler_time_qos>(std::get<time_qos_scheduler_config>(policy_cfg), cell_cfg);
  }
  return nullptr;
}
