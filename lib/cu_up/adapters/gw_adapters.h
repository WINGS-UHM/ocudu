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

#include "ocudu/f1u/cu_up/f1u_gateway.h"
#include "ocudu/f1u/cu_up/f1u_tx_pdu_notifier.h"
#include "ocudu/gateways/network_gateway.h"
#include "ocudu/gtpu/gtpu_demux.h"

namespace ocudu::ocuup {

/// Adapter between Network Gateway (Data) and GTP-U demux
class network_gateway_data_gtpu_demux_adapter : public ocudu::network_gateway_data_notifier_with_src_addr
{
public:
  network_gateway_data_gtpu_demux_adapter()           = default;
  ~network_gateway_data_gtpu_demux_adapter() override = default;

  void connect_gtpu_demux(gtpu_demux_rx_upper_layer_interface& gtpu_demux_) { gtpu_demux = &gtpu_demux_; }

  void disconnect() { gtpu_demux = nullptr; }

  void on_new_pdu(byte_buffer pdu, const sockaddr_storage& src_addr) override
  {
    if (gtpu_demux != nullptr) {
      gtpu_demux->handle_pdu(std::move(pdu), src_addr);
    } else {
      ocudulog::fetch_basic_logger("GTPU", false).debug("Dropped DL GTP-U PDU. Demux adapter is disconnected.");
    }
  }

private:
  gtpu_demux_rx_upper_layer_interface* gtpu_demux = nullptr;
};

} // namespace ocudu::ocuup
