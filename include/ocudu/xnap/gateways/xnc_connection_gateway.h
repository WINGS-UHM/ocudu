// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/support/async/async_task.h"
#include "ocudu/support/io/transport_layer_address.h"
#include "ocudu/xnap/xnap_message_notifier.h"
#include <memory>

namespace ocudu::ocucp {

class cu_cp_xnc_handler;

/// Connection gateway responsible for handling new connection requests/drops coming
/// from neighbour gNBs via the XN-C interface and converting them to CU-CP commands.
class xnc_connection_gateway
{
public:
  virtual ~xnc_connection_gateway() = default;

  /// Get the initial XNAP message notifier for initiating the SCTP association with peer.
  virtual std::unique_ptr<xnap_message_notifier> get_init_tx_notifier(transport_layer_address peer_addr) = 0;

  /// Initiate a connection to a peer.
  /// \return async_task that resolves to true when SCTP association is ready, false on failure.
  virtual async_task<bool> connect_to_peer(transport_layer_address peer_addr) = 0;

  /// Attach a CU-CP handler to the XN-C connection server.
  virtual void attach_cu_cp(cu_cp_xnc_handler& xnc_handler_) = 0;

  /// Stop the XN-C connection gateway.
  virtual void stop() = 0;

  /// Get port on which the XN-C Server is listening for new connections.
  ///
  /// This method is useful in testing, where we don't want to use a specific port.
  /// \return The port number on which the XN-C Server is listening for new connections.
  virtual std::optional<uint16_t> get_listen_port() const = 0;
};

} // namespace ocudu::ocucp
