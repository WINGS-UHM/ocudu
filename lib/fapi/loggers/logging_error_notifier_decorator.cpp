// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#include "logging_error_notifier_decorator.h"
#include "decorator_helpers/error_notifier_dummy.h"
#include "message_loggers.h"

using namespace ocudu;
using namespace fapi;

static error_notifier_dummy dummy_notifier;

logging_error_notifier_decorator::logging_error_notifier_decorator(unsigned                sector_id_,
                                                                   ocudulog::basic_logger& logger_) :
  sector_id(sector_id_), logger(logger_), notifier(&dummy_notifier)
{
}

void logging_error_notifier_decorator::on_error_indication(const error_indication& msg)
{
  log_error_indication(msg, sector_id, logger);

  notifier->on_error_indication(msg);
}

void logging_error_notifier_decorator::set_error_indication_notifier(error_indication_notifier& error_notifier)
{
  notifier = &error_notifier;
}
