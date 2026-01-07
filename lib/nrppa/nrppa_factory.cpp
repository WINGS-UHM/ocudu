/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/nrppa/nrppa_factory.h"
#include "nrppa_impl.h"

/// Notice this would be the only place were we include concrete class implementation files.

using namespace ocudu;
using namespace ocucp;

std::unique_ptr<nrppa_interface> ocudu::ocucp::create_nrppa(const cu_cp_configuration& cfg,
                                                            nrppa_cu_cp_notifier&      cu_cp_notifier,
                                                            common_task_scheduler&     common_task_sched)
{
  auto nrppa = std::make_unique<nrppa_impl>(cfg, cu_cp_notifier, common_task_sched);
  return nrppa;
}
