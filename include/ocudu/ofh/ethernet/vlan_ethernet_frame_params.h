// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

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
