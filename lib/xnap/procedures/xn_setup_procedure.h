// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "../xnap_tx_pdu_notifier_with_log.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/async/async_task.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_configuration.h"
#include "ocudu/xnap/xnap_message.h"

namespace ocudu::ocucp {

class xn_setup_procedure
{
public:
  xn_setup_procedure(const xnap_configuration&          xnap_cfg,
                     xnap_tx_pdu_notifier_with_logging& tx_notifier_,
                     ocudulog::basic_logger&            logger_);

  void operator()(coro_context<async_task<void>>& ctx);

  static const char* name() { return "XN Setup Procedure"; }

private:
  void send_xn_setup_message();

  const xnap_configuration&          xnap_cfg;
  xnap_tx_pdu_notifier_with_logging& tx_notifier;
  ocudulog::basic_logger&            logger;

  xnap_message xn_setup_req;
};

} // namespace ocudu::ocucp
