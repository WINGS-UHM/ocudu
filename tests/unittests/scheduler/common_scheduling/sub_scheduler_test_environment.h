// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "lib/scheduler/cell/resource_grid.h"
#include "lib/scheduler/config/cell_configuration.h"
#include "lib/scheduler/logging/scheduler_event_logger.h"
#include "lib/scheduler/logging/scheduler_metrics_handler.h"
#include "lib/scheduler/logging/scheduler_result_logger.h"
#include "tests/test_doubles/utils/test_rng.h"
#include "ocudu/scheduler/config/scheduler_expert_config_factory.h"

namespace ocudu {

/// Common base class used by different types of sub-scheduler tests.
class sub_scheduler_test_environment
{
private:
  virtual void do_run_slot() = 0;

protected:
  sub_scheduler_test_environment(const sched_cell_configuration_request_message& cell_req);
  virtual ~sub_scheduler_test_environment() = default;

  void run_slot();

  /// Helper function to flush pending events and logs.
  void flush_events();

  ocudulog::basic_logger& mac_logger  = ocudulog::fetch_basic_logger("SCHED", true);
  ocudulog::basic_logger& test_logger = ocudulog::fetch_basic_logger("TEST", true);

  scheduler_expert_config sched_cfg{config_helpers::make_default_scheduler_expert_config()};
  cell_configuration      cell_cfg;
  scheduler_event_logger  ev_logger{cell_cfg.cell_index, cell_cfg.params.pci};
  cell_metrics_handler    metrics_hdlr{cell_cfg, std::nullopt};
  cell_resource_allocator res_grid{cell_cfg};
  scheduler_result_logger result_logger{true, cell_cfg.params.pci};

  // -- Derived
  /// Maximum of all k values.
  /// \note We use this value to account for the case when the PDSCH or PUSCH is allocated several slots in advance.
  uint8_t max_k_value = 0;

  // -- State

  // Slot of the next call to the scheduler.
  slot_point next_slot{to_numerology_value(cell_cfg.scs_common()),
                       test_rng::uniform_int<unsigned>(
                           0,
                           ((NOF_SFNS * NOF_SUBFRAMES_PER_FRAME) << to_numerology_value(cell_cfg.scs_common())) - 1)};
};

} // namespace ocudu
