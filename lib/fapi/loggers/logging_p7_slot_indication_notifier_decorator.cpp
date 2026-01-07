/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "logging_p7_slot_indication_notifier_decorator.h"
#include "decorator_helpers/slot_indication_notifier_dummy.h"
#include "message_loggers.h"

using namespace ocudu;
using namespace fapi;

static slot_indication_notifier_dummy dummy_notifier;

logging_p7_slot_indication_notifier_decorator::logging_p7_slot_indication_notifier_decorator(
    unsigned                sector_id_,
    ocudulog::basic_logger& logger_) :
  sector_id(sector_id_), logger(logger_), p7_notifier(&dummy_notifier)
{
}

void logging_p7_slot_indication_notifier_decorator::on_slot_indication(const slot_indication& msg)
{
  log_slot_indication(msg, sector_id, logger);

  p7_notifier->on_slot_indication(msg);
}

void logging_p7_slot_indication_notifier_decorator::set_p7_slot_indication_notifier(
    p7_slot_indication_notifier& p7_slot_ind_notifier)
{
  p7_notifier = &p7_slot_ind_notifier;
}
