// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/cu_cp/o_cu_cp_factory.h"
#include "o_cu_cp_impl.h"
#include "ocudu/cu_cp/cu_cp_factory.h"
#include "ocudu/cu_cp/o_cu_cp_config.h"
#include "ocudu/e2/e2_cu_cp_factory.h"

using namespace ocudu;
using namespace ocucp;

std::unique_ptr<o_cu_cp> ocucp::create_o_cu_cp(const o_cu_cp_config& config, const o_cu_cp_dependencies& dependencies)
{
  auto cu = create_cu_cp(config.cu_cp_config);

  if (dependencies.e2_client == nullptr) {
    return std::make_unique<o_cu_cp_impl>(std::move(cu));
  }

  auto e2agent = create_e2_cu_cp_agent(
      config.e2ap_config,
      *dependencies.e2_client,
      dependencies.e2_cu_metric_iface,
      &cu.get()->get_cu_configurator(),
      timer_factory{*config.cu_cp_config.services.timers, *config.cu_cp_config.services.cu_cp_e2_exec},
      *config.cu_cp_config.services.cu_cp_e2_exec);

  return std::make_unique<o_cu_cp_with_e2_impl>(std::move(e2agent), std::move(cu));
}
