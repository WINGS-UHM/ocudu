/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/gateways/udp_network_gateway_factory.h"
#include "udp_network_gateway_impl.h"

using namespace ocudu;

std::unique_ptr<udp_network_gateway> ocudu::create_udp_network_gateway(const udp_network_gateway_creation_message& msg)
{
  return std::make_unique<udp_network_gateway_impl>(
      msg.config, msg.data_notifier, msg.io_tx_executor, msg.io_rx_executor);
}
