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

#include "ocudu/f1u/du/f1u_gateway.h"
#include "ocudu/f1u/split_connector/f1u_five_qi_gw_maps.h"
#include "ocudu/gtpu/gtpu_demux.h"
#include "ocudu/pcap/dlt_pcap.h"
#include <cstdint>

namespace ocudu::odu {

struct f1u_du_split_gateway_creation_msg {
  const gtpu_gateway_maps& udp_gw_maps;
  gtpu_demux*              demux;
  dlt_pcap&                gtpu_pcap;
  uint16_t                 peer_port;
};

std::unique_ptr<f1u_du_udp_gateway> create_split_f1u_gw(f1u_du_split_gateway_creation_msg msg);

} // namespace ocudu::odu
