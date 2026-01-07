/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "p7_indications_notifier_dispatcher.h"
#include "p7_indications_notifier_dummy.h"

using namespace ocudu;
using namespace fapi;

static p7_indications_notifier_dummy dummy_notifier;

p7_indications_notifier_dispatcher::p7_indications_notifier_dispatcher() : notifier(&dummy_notifier) {}

void p7_indications_notifier_dispatcher::on_rx_data_indication(const rx_data_indication& msg)
{
  notifier->on_rx_data_indication(msg);
}

void p7_indications_notifier_dispatcher::on_crc_indication(const crc_indication& msg)
{
  notifier->on_crc_indication(msg);
}

void p7_indications_notifier_dispatcher::on_uci_indication(const uci_indication& msg)
{
  notifier->on_uci_indication(msg);
}

void p7_indications_notifier_dispatcher::on_srs_indication(const srs_indication& msg)
{
  notifier->on_srs_indication(msg);
}

void p7_indications_notifier_dispatcher::on_rach_indication(const rach_indication& msg)
{
  notifier->on_rach_indication(msg);
}

void p7_indications_notifier_dispatcher::set_p7_indications_notifier(p7_indications_notifier& p7_notifier)
{
  notifier = &p7_notifier;
}
