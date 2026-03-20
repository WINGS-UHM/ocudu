// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/gateways/sctp_network_client.h"
#include "ocudu/support/async/async_task.h"
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

  /// \brief Stop the server, cancelling any pending deferred tasks and cleaning up associations.
  virtual void stop() = 0;

  /// \brief Start listening for new SCTP associations.
  virtual bool listen() = 0;

  /// \brief Get port to which server binded and is listening for connections.
  virtual std::optional<uint16_t> get_listen_port() = 0;

  /// \brief Initiate new SCTP association to peer.
  virtual bool init_association_with_msg(transport_layer_address dest_addr, byte_buffer payload) = 0;

  /// \brief Initiate a new SCTP association to a peer in a non-blocking way via sctp_connectx().
  ///
  /// Returns an async_task<bool> that will complete with:
  /// - true on SCTP_COMM_UP, followed by sctp_network_association_factory::create() callback
  /// - false on SCTP_CANT_STR_ASSOC, immediate sctp_connectx failure, or if a connection to the same address is
  ///   already pending.
  virtual async_task<bool> connect(transport_layer_address dest_addr) = 0;
};

} // namespace ocudu
