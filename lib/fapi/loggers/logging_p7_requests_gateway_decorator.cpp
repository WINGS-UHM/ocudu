/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "logging_p7_requests_gateway_decorator.h"
#include "message_loggers.h"

using namespace ocudu;
using namespace fapi;

void logging_p7_requests_gateway_decorator::send_dl_tti_request(const dl_tti_request& msg)
{
  log_dl_tti_request(msg, sector_id, logger);

  p7_gateway.send_dl_tti_request(msg);
}

void logging_p7_requests_gateway_decorator::send_ul_tti_request(const ul_tti_request& msg)
{
  log_ul_tti_request(msg, sector_id, logger);

  p7_gateway.send_ul_tti_request(msg);
}

void logging_p7_requests_gateway_decorator::send_ul_dci_request(const ul_dci_request& msg)
{
  log_ul_dci_request(msg, sector_id, logger);

  p7_gateway.send_ul_dci_request(msg);
}

void logging_p7_requests_gateway_decorator::send_tx_data_request(const tx_data_request& msg)
{
  log_tx_data_request(msg, sector_id, logger);

  p7_gateway.send_tx_data_request(msg);
}
