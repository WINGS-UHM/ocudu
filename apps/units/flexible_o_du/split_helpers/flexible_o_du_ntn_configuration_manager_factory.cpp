/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "flexible_o_du_ntn_configuration_manager_factory.h"
#include "ocudu/ntn/ntn_configuration_manager.h"

using namespace ocudu;

#ifndef OCUDU_HAS_ENTERPRISE_NTN

std::unique_ptr<ocudu_ntn::ntn_configuration_manager>
ocudu::create_ntn_configuration_manager(const ocudu_ntn::ntn_configuration_manager_config& ntn_config,
                                        odu::du_configurator&                              du_cfgtr,
                                        odu::du_manager_time_mapper_accessor&              du_time_mapper_accessor,
                                        ru_controller&                                     ru_ctrl,
                                        timer_manager&                                     timers,
                                        task_executor&                                     executor)
{
  return nullptr;
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
