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

#include "tests/unittests/cu_cp/test_helpers.h"
#include "ocudu/support/executors/manual_task_worker.h"
#include <gtest/gtest.h>

namespace ocudu {
namespace ocucp {

/// Fixture class for CU-CP mobility tests
class mobility_test : public ::testing::Test
{
protected:
  mobility_test();
  ~mobility_test() override;

  ue_manager* get_ue_manager() { return &ue_mng; }

  ocudulog::basic_logger& test_logger  = ocudulog::fetch_basic_logger("TEST");
  ocudulog::basic_logger& cu_cp_logger = ocudulog::fetch_basic_logger("CU-CP");

  manual_task_worker  ctrl_worker{128};
  timer_manager       timers;
  cu_cp_configuration cu_cp_cfg;

  ue_manager                                  ue_mng{cu_cp_cfg};
  dummy_cu_cp_ue_context_manipulation_handler cu_cp_handler;
};

} // namespace ocucp
} // namespace ocudu
