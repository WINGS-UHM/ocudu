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

#include "ocudu/gtpu/gtpu_config.h"
#include "ocudu/gtpu/gtpu_tunnel_common_tx.h"
#include "ocudu/gtpu/gtpu_tunnel_ngu.h"
#include "ocudu/gtpu/gtpu_tunnel_ngu_rx.h"
#include "ocudu/pcap/dlt_pcap.h"
#include "ocudu/ran/cu_up_types.h"
#include "ocudu/support/timers.h"
#include <memory>

/// This factory header file depends on the GTP-U tunnel interfaces (see above include list). It is kept separate as
/// clients of the GTP-U tunnel interfaces do not need to call factory methods.
namespace ocudu {

struct gtpu_tunnel_ngu_creation_message {
  cu_up_ue_index_t                            ue_index;
  gtpu_tunnel_ngu_config                      cfg;
  dlt_pcap*                                   gtpu_pcap;
  gtpu_tunnel_ngu_rx_lower_layer_notifier*    rx_lower;
  gtpu_tunnel_common_tx_upper_layer_notifier* tx_upper;
  timer_factory                               ue_ctrl_timer_factory;
};

/// Creates an instance of a GTP-U entity.
std::unique_ptr<gtpu_tunnel_ngu> create_gtpu_tunnel_ngu(gtpu_tunnel_ngu_creation_message& msg);

} // namespace ocudu
