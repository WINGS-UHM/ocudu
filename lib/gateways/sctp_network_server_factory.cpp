/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/gateways/sctp_network_server_factory.h"
#include "sctp_network_server_impl.h"

using namespace ocudu;

std::unique_ptr<sctp_network_server> ocudu::create_sctp_network_server(const sctp_network_server_config& config)
{
  return sctp_network_server_impl::create(
      config.sctp, config.broker, config.io_rx_executor, config.association_handler_factory);
}
