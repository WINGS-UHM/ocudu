/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mobility_test_helpers.h"
#include "ocudu/cu_cp/cu_cp_configuration_helpers.h"

using namespace ocudu;
using namespace ocucp;

mobility_test::mobility_test() :
  cu_cp_cfg([this]() {
    cu_cp_configuration cucfg     = config_helpers::make_default_cu_cp_config();
    cucfg.services.timers         = &timers;
    cucfg.services.cu_cp_executor = &ctrl_worker;
    return cucfg;
  }())
{
  test_logger.set_level(ocudulog::basic_levels::debug);
  cu_cp_logger.set_level(ocudulog::basic_levels::debug);
  ocudulog::init();
}

mobility_test::~mobility_test()
{
  // flush logger after each test
  ocudulog::flush();
}
