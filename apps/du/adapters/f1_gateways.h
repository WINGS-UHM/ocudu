/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/f1ap/gateways/f1c_network_client_factory.h"

namespace ocudu {

/// Instantiates an F1-C DU client.
inline std::unique_ptr<odu::f1c_connection_client>
create_f1c_client_gateway(const std::vector<std::string>& cu_cp_addresses,
                          const std::vector<std::string>& bind_addresses,
                          io_broker&                      broker,
                          task_executor&                  io_rx_executor,
                          dlt_pcap&                       f1ap_pcap)
{
  sctp_network_connector_config f1c_sctp{};
  f1c_sctp.if_name           = "F1-C";
  f1c_sctp.dest_name         = "CU-CP";
  f1c_sctp.connect_addresses = cu_cp_addresses;
  f1c_sctp.connect_port      = F1AP_PORT;
  f1c_sctp.ppid              = F1AP_PPID;
  f1c_sctp.bind_addresses    = bind_addresses;

  return create_f1c_gateway_client(f1c_du_sctp_gateway_config{f1c_sctp, broker, io_rx_executor, f1ap_pcap});
}

} // namespace ocudu
