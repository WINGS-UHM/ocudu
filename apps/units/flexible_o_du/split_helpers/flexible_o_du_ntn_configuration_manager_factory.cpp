// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "flexible_o_du_ntn_configuration_manager_factory.h"
#include "ocudu/ntn/ntn_configuration_manager.h"

using namespace ocudu;

#ifndef OCUDU_HAS_ENTERPRISE_NTN

std::unique_ptr<ocudu_ntn::ntn_configuration_manager>
ocudu::create_ntn_configuration_manager(const ocudu_ntn::ntn_configuration_manager_config& ntn_config,
                                        odu::du_configurator&                              du_cfgtr,
                                        mac_subframe_time_mapper&                          du_time_mapper_accessor,
                                        ru_controller&                                     ru_ctrl,
                                        timer_manager&                                     timers,
                                        task_executor&                                     executor)
{
  return nullptr;
}

#endif // OCUDU_HAS_ENTERPRISE_NTN
