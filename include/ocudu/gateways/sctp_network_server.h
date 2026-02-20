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

#include "ocudu/gateways/sctp_network_client.h"
#include "ocudu/support/io/transport_layer_address.h"

namespace ocudu {

struct sctp_association_info {
  int                     assoc_id;
  transport_layer_address peer_addr;
};

/// Factory of new SCTP association handlers.
class sctp_network_association_factory
{
public:
  virtual ~sctp_network_association_factory() = default;

  /// Called on every SCTP association notification, to create a new SCTP association handler.
  virtual std::unique_ptr<sctp_association_sdu_notifier>
  create(std::unique_ptr<sctp_association_sdu_notifier> sctp_send_notifier, sctp_association_info assoc_info) = 0;
};

/// SCTP network server interface, which will handle requests to start new SCTP associations.
class sctp_network_server : public sctp_network_gateway_info
{
public:
  virtual ~sctp_network_server() = default;

  /// \brief Start listening for new SCTP associations.
  virtual bool listen() = 0;

  /// \brief Get port to which server binded and is listening for connections.
  virtual std::optional<uint16_t> get_listen_port() = 0;

  /// \brief Initiate new SCTP association to peer.
  virtual bool init_association_with_msg(transport_layer_address dest_addr, byte_buffer payload) = 0;
};

} // namespace ocudu
