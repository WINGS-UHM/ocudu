/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
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
                                                                   du_cell_index_t                cell_index)
{
  if (std::holds_alternative<time_rr_scheduler_config>(policy_cfg)) {
    return std::make_unique<scheduler_time_rr>();
  }
  if (std::holds_alternative<time_qos_scheduler_config>(policy_cfg)) {
    return std::make_unique<scheduler_time_qos>(std::get<time_qos_scheduler_config>(policy_cfg), cell_index);
  }
  return nullptr;
}
