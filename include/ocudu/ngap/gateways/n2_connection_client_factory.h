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

#include "ocudu/gateways/sctp_network_gateway.h"
#include "ocudu/ngap/gateways/n2_connection_client.h"
#include <chrono>
#include <variant>

namespace ocudu {

class io_broker;
class dlt_pcap;

namespace ocucp {

/// Parameters for the NG gateway instantiation.
struct n2_connection_client_config {
  /// Parameters for a local AMF stub connection.
  struct no_core {};

  /// Parameters specific to an SCTP network gateway.
  struct network {
    io_broker&                               broker;
    task_executor&                           io_rx_executor;
    std::vector<std::string>                 bind_addresses;
    std::string                              bind_interface;
    std::vector<std::string>                 amf_addresses;
    int                                      amf_port = NGAP_PORT;
    std::optional<std::chrono::milliseconds> rto_initial;
    std::optional<std::chrono::milliseconds> rto_min;
    std::optional<std::chrono::milliseconds> rto_max;
    std::optional<int32_t>                   init_max_attempts;
    std::optional<std::chrono::milliseconds> max_init_timeo;
    std::optional<std::chrono::milliseconds> hb_interval;
    std::optional<int32_t>                   assoc_max_rxt;
    std::optional<bool>                      nodelay;
  };

  /// PCAP writer for the NGAP messages.
  dlt_pcap& pcap;

  /// Mode of operation.
  std::variant<no_core, network> mode;
};

/// Create an N2 connection client.
std::unique_ptr<n2_connection_client> create_n2_connection_client(const n2_connection_client_config& params);

} // namespace ocucp
} // namespace ocudu
