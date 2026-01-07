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

#include "ocudu/ran/pci.h"
#include "ocudu/ran/prach/prach_format_type.h"

namespace ocudu {
namespace fapi {

enum class prach_config_scope_type : uint8_t { common_context, phy_context };

/// PRACH maintenance parameters added in FAPIv3.
struct ul_prach_maintenance_v3 {
  uint32_t                handle;
  prach_config_scope_type prach_config_scope;
  uint16_t                prach_res_config_index;
  uint8_t                 num_fd_ra;
  uint8_t                 start_preamble_index;
  uint8_t                 num_preamble_indices;
};

/// Uplink PRACH PDU information.
struct ul_prach_pdu {
  pci_t             phys_cell_id;
  uint8_t           num_prach_ocas;
  prach_format_type prach_format;
  uint8_t           index_fd_ra;
  uint8_t           prach_start_symbol;
  uint16_t          num_cs;
  uint8_t           is_msg_a_prach;
  bool              has_msg_a_pusch_beamforming;
  //: TODO: beamforming struct
  ul_prach_maintenance_v3 maintenance_v3;
  //: TODO: uplink spatial assignment struct
  //: TODO: msgA signalling in v4
  //: TODO: msgA pusch beamforming
};

} // namespace fapi
} // namespace ocudu
