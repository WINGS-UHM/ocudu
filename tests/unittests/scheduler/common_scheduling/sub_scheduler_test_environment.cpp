// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "sub_scheduler_test_environment.h"
#include "lib/scheduler/logging/scheduler_metrics_handler.h"
#include "lib/scheduler/logging/scheduler_result_logger.h"
#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include <gtest/gtest.h>

using namespace ocudu;

sub_scheduler_test_environment::sub_scheduler_test_environment(
    const sched_cell_configuration_request_message& cell_req) :
  cell_cfg(sched_cfg, cell_req)
{
  mac_logger.set_level(ocudulog::basic_levels::debug);
  test_logger.set_level(ocudulog::basic_levels::info);
  ocudulog::init();

  // Derive max_k_value.
  const auto& dl_lst = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
  for (const auto& pdsch : dl_lst) {
    max_k_value = std::max<unsigned>(pdsch.k0, max_k_value);
  }
  const auto&        ul_lst         = cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;
  constexpr unsigned max_msg3_delta = 6;
  for (const auto& pusch : ul_lst) {
    max_k_value = std::max<unsigned>(max_k_value, pusch.k2 + max_msg3_delta);
  }
}

void sub_scheduler_test_environment::run_slot()
{
  mac_logger.set_context(next_slot.sfn(), next_slot.slot_index());
  test_logger.set_context(next_slot.sfn(), next_slot.slot_index());

  res_grid.slot_indication(next_slot);

  // Run slot for the derived class.
  do_run_slot();

  result_logger.on_scheduler_result(res_grid[0].result);

  // Check sched result consistency.
  ASSERT_NO_FATAL_FAILURE(test_scheduler_result_consistency(cell_cfg, res_grid));
  ++next_slot;
}

void sub_scheduler_test_environment::flush_events()
{
  // Log pending allocations before finishing test.
  for (unsigned i = 0; i != max_k_value; ++i) {
    run_slot();
  }
  ocudulog::flush();
}
