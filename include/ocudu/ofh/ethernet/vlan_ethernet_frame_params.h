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

#include "ocudu/ofh/ethernet/ethernet_mac_address.h"
#include <optional>

namespace ocudu {
namespace ether {

/// VLAN Ethernet frame header parameters.
struct vlan_frame_params {
  /// Destination MAC address.
  mac_address mac_dst_address;
  /// Source MAC address.
  mac_address mac_src_address;
  /// Tag control information field.
  std::optional<uint16_t> tci;
  /// Ethernet type field.
  uint16_t eth_type;
};

} // namespace ether
} // namespace ocudu
