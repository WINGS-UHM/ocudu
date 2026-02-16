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

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/support/io/transport_layer_address.h"

namespace ocudu::ocucp {

/// Connection gateway responsible for handling new connection requests/drops coming
/// from neighbour gNBs via the XN-C interface and converting them to CU-CP commands.
class xnc_connection_gateway
{
public:
  virtual ~xnc_connection_gateway() = default;

  /// Initiate SCTP association with peer.
  virtual void init_association(transport_layer_address dest_addr, byte_buffer payload) = 0;
};

} // namespace ocudu::ocucp
