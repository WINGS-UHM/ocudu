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
#include "ocudu/ofh/ethernet/ethernet_frame_builder.h"
#include "ocudu/ofh/ethernet/ethernet_receiver.h"
#include "ocudu/ofh/ethernet/ethernet_receiver_config.h"
#include "ocudu/ofh/ethernet/ethernet_transmitter.h"
#include "ocudu/ofh/ethernet/ethernet_transmitter_config.h"
#include "ocudu/ofh/ethernet/vlan_ethernet_frame_decoder.h"
#include <memory>

namespace ocudu {

class task_executor;

namespace ether {

class frame_notifier;

/// Creates an Ethernet transmitter.
std::unique_ptr<transmitter> create_transmitter(const transmitter_config& config, ocudulog::basic_logger& logger);

/// Creates an Ethernet receiver.
std::unique_ptr<receiver>
create_receiver(const receiver_config& config, task_executor& executor, ocudulog::basic_logger& logger);

/// Creates an Ethernet frame builder with VLAN tag insertion.
std::unique_ptr<frame_builder> create_vlan_frame_builder(const vlan_frame_params& eth_params);

/// Creates an Ethernet frame builder without VLAN tag insertion.
std::unique_ptr<frame_builder> create_frame_builder(const vlan_frame_params& eth_params);

/// Creates an Ethernet VLAN frame decoder.
std::unique_ptr<vlan_frame_decoder> create_vlan_frame_decoder(ocudulog::basic_logger& logger, unsigned sector_id);

} // namespace ether
} // namespace ocudu
