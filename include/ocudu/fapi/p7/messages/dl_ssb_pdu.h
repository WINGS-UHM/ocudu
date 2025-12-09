/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/ssb/ssb_properties.h"

namespace ocudu {
namespace fapi {

enum class dmrs_typeA_pos : uint8_t { pos2 = 0, pos3 };

/// PHY generated MIB PDU information.
struct dl_ssb_phy_mib_pdu {
  dmrs_typeA_pos dmrs_typeA_position;
  uint8_t        pdcch_config_sib1;
  bool           cell_barred;
  bool           intrafreq_reselection;
};

/// BCH payload information.
struct dl_ssb_bch_payload {
  union {
    uint32_t           bch_payload;
    dl_ssb_phy_mib_pdu phy_mib_pdu;
  };
};

/// SSB/PBCH maintenance parameters added in FAPIv3.
struct dl_ssb_maintenance_v3 {
  uint8_t            ssb_pdu_index;
  ssb_pattern_case   case_type;
  subcarrier_spacing scs;
  uint8_t            L_max;
};

/// PSS EPRE to SSS EPRE in a SS/PBCH block.
enum class beta_pss_profile_type : uint8_t { dB_0 = 0, dB_3 = 1, beta_pss_profile_sss = 255 };

/// BCH payload generation options.
enum class bch_payload_type : uint8_t { mac_full, phy_timing_info, phy_full };

/// Downlink SSB PDU information.
struct dl_ssb_pdu {
  pci_t                            phys_cell_id;
  beta_pss_profile_type            beta_pss_profile_nr;
  uint8_t                          ssb_block_index;
  uint8_t                          ssb_subcarrier_offset;
  ssb_offset_to_pointA             ssb_offset_pointA;
  bch_payload_type                 bch_payload_flag;
  dl_ssb_bch_payload               bch_payload;
  tx_precoding_and_beamforming_pdu precoding_and_beamforming;
  dl_ssb_maintenance_v3            ssb_maintenance_v3;
  //: TODO: params v4 - MU-MIMO
};

} // namespace fapi
} // namespace ocudu
