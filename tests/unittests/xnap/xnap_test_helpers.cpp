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

  auto assoc = std::make_unique<dummy_xnap_message_notifier>();
  xnap       = std::make_unique<xnap_impl>(xnap_local_cfg, xnc_gw, ctrl_worker);
  tx_assoc   = assoc.get();
  xnap->set_tx_association_notifier(std::move(assoc));
}

void xnap_test::TearDown()
{
  // Flush logger after each test.
  ocudulog::flush();
  tx_assoc = nullptr;
}

std::optional<xnap_message> xnap_test::get_last_message()
{
  if (tx_assoc == nullptr) {
    return std::nullopt;
  }
  return tx_assoc->last_msg;
}
