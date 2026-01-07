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

#include "ocudu/ocudulog/logger.h"
#include "ocudu/ofh/ethernet/vlan_ethernet_frame_decoder.h"

namespace ocudu {
namespace ether {

/// Implementation for the VLAN Ethernet frame decoder.
class vlan_frame_decoder_impl : public vlan_frame_decoder
{
public:
  vlan_frame_decoder_impl(ocudulog::basic_logger& logger_, unsigned sector_id_) : logger(logger_), sector_id(sector_id_)
  {
  }

  // See interface for documentation.
  span<const uint8_t> decode(span<const uint8_t> frame, vlan_frame_params& eth_params) override;

private:
  ocudulog::basic_logger& logger;
  const unsigned          sector_id;
};

} // namespace ether
} // namespace ocudu
