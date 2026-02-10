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

#include "ocudu/adt/batched_dispatch_queue.h"
#include "ocudu/adt/byte_buffer.h"
#include "ocudu/gtpu/gtpu_teid.h"
#include "ocudu/gtpu/gtpu_tunnel_common_rx.h"
#include "ocudu/gtpu/gtpu_tunnel_common_tx.h"
#include "ocudu/support/executors/task_executor.h"
#include <sys/socket.h>

namespace ocudu {

struct gtpu_demux_cfg_t {
  bool     warn_on_drop;
  bool     test_mode  = false;
  uint32_t queue_size = 8192;
  uint32_t batch_size = 256;
};

struct gtpu_demux_pdu_ctx_t {
  byte_buffer      pdu;
  sockaddr_storage src_addr;
};

using gtpu_demux_dispatch_queue = batched_dispatch_queue<gtpu_demux_pdu_ctx_t>;

/// The GTP-U demux component will only be relevant for the reception and de-multiplexing
/// of GTP-U packets. It does not require Tx capabilities as the corresponding GTP-U entities
/// will already be calling the corresponding UDP network gateway.

/// This interface is used to push incoming GTP-U messages to the demuxer.
class gtpu_demux_rx_upper_layer_interface
{
public:
  virtual ~gtpu_demux_rx_upper_layer_interface() = default;

  /// \brief Interface for the IO gateway to pass PDUs into the GTP-U demuxer.
  virtual void handle_pdu(byte_buffer pdu, const sockaddr_storage& src_addr) = 0;
};

/// \brief This interface manages the TEID to GTP-U tunnel mapping.
class gtpu_demux_ctrl
{
public:
  virtual ~gtpu_demux_ctrl() = default;

  /// Add a new TEID to GTP-U tunnel mapping.
  [[nodiscard]] virtual expected<std::unique_ptr<gtpu_demux_dispatch_queue>>
  add_tunnel(gtpu_teid_t teid, task_executor& tunnel_exec, gtpu_tunnel_common_rx_upper_layer_interface* tunnel) = 0;

  /// \brief Remove TEID from mapping.
  virtual bool remove_tunnel(gtpu_teid_t teid) = 0;

  /// \brief Apply a new TEID when in test mode.
  virtual void apply_test_teid(gtpu_teid_t teid) = 0;

  /// \brief Mark GTP-U demux as stopped.
  virtual void stop() = 0;

  /// \brief Set the TX notifier and local address for sending GTP-U Error Indication messages.
  /// Ref: TS 29.281 Sec. 7.3.1.
  virtual void set_error_indication_tx(gtpu_tunnel_common_tx_upper_layer_notifier& tx_upper,
                                       const std::string&                          local_addr) = 0;
};

/// Combined entry point for the GTPU-demux object.
class gtpu_demux : public gtpu_demux_rx_upper_layer_interface, public gtpu_demux_ctrl
{
public:
  virtual ~gtpu_demux() = default;
};

} // namespace ocudu
