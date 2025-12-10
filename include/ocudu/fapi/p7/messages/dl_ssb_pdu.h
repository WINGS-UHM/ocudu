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
#include "ocudu/ran/ssb/ssb_configuration.h"

namespace ocudu {
namespace fapi {

/// PSS EPRE to SSS EPRE in a SS/PBCH block.
enum class beta_pss_profile_type : uint8_t { dB_0 = 0, dB_3 = 1, beta_pss_profile_sss = 255 };

/// Downlink SSB PDU information.
struct dl_ssb_pdu {
  pci_t                 phys_cell_id;
  beta_pss_profile_type beta_pss_profile_nr;
  ssb_id_t              ssb_block_index;
  ssb_subcarrier_offset subcarrier_offset;
  ssb_offset_to_pointA  ssb_offset_pointA;
  /// \note Payload encoded as: 0,0,0,0,0,0,0,0,a0,a1,a2,...,a21,a22,a23, with the most significant bit being the
  /// leftmost (in this case a0 in position 24 of the uint32_t).
  /// \note The timing bits should be added by the underlying PHY.
  uint32_t           bch_payload;
  ssb_pattern_case   case_type;
  subcarrier_spacing scs;
  uint8_t            L_max;
};

} // namespace fapi
} // namespace ocudu
