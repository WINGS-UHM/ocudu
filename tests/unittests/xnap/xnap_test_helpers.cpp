/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xnap_test_helpers.h"

using namespace ocudu;
using namespace ocucp;

void xnap_test::SetUp()
{
  // Init test's loggers.
  ocudulog::init();
  logger.set_level(ocudulog::basic_levels::debug);

  ocudulog::fetch_basic_logger("XNAP", false).set_level(ocudulog::basic_levels::debug);
  ocudulog::fetch_basic_logger("XNAP", false).set_hex_dump_max_size(100);

  xnap =
      std::make_unique<xnap_impl>(xnap_local_cfg,
                                  xnc_gw.get_init_tx_notifier(transport_layer_address::create_from_string("127.0.0.1")),
                                  ctrl_worker);
}

void xnap_test::TearDown()
{
  // Flush logger after each test.
  ocudulog::flush();
}
