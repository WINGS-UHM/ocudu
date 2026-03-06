// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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
                                  cu_cp_notifier,
                                  xnc_gw.get_init_tx_notifier(transport_layer_address::create_from_string("127.0.0.1")),
                                  timers,
                                  ctrl_worker);
}

void xnap_test::TearDown()
{
  // Flush logger after each test.
  ocudulog::flush();
}
