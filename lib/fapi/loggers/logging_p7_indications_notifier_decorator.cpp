/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "logging_p7_indications_notifier_decorator.h"
#include "decorator_helpers/p7_indications_notifier_dummy.h"
#include "message_loggers.h"

using namespace ocudu;
using namespace fapi;

static p7_indications_notifier_dummy dummy_notifier;

logging_p7_indications_notifier_decorator::logging_p7_indications_notifier_decorator(unsigned                sector_id_,
                                                                                     ocudulog::basic_logger& logger_) :
  sector_id(sector_id_), logger(logger_), notifier(&dummy_notifier)
{
}

void logging_p7_indications_notifier_decorator::on_rx_data_indication(const rx_data_indication& msg)
{
  log_rx_data_indication(msg, sector_id, logger);

  notifier->on_rx_data_indication(msg);
}

void logging_p7_indications_notifier_decorator::on_crc_indication(const crc_indication& msg)
{
  log_crc_indication(msg, sector_id, logger);

  notifier->on_crc_indication(msg);
}

void logging_p7_indications_notifier_decorator::on_uci_indication(const uci_indication& msg)
{
  log_uci_indication(msg, sector_id, logger);

  notifier->on_uci_indication(msg);
}

void logging_p7_indications_notifier_decorator::on_srs_indication(const srs_indication& msg)
{
  log_srs_indication(msg, sector_id, logger);

  notifier->on_srs_indication(msg);
}

void logging_p7_indications_notifier_decorator::on_rach_indication(const rach_indication& msg)
{
  log_rach_indication(msg, sector_id, logger);

  notifier->on_rach_indication(msg);
}

void logging_p7_indications_notifier_decorator::set_p7_indications_notifier(p7_indications_notifier& p7_notifier)
{
  notifier = &p7_notifier;
}
