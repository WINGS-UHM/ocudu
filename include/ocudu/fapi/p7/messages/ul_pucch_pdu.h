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

#include "ocudu/fapi/p7/messages/uci_part1_to_part2_correspondence_v3.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/pucch/pucch_mapping.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace fapi {

/// PUCCH PDU maintenance information added in FAPIv3.
struct ul_pucch_maintenance_v3 {
  uint8_t max_code_rate;
  uint8_t ul_bwp_id;
};

/// Encodes PUCCH pdu.
struct ul_pucch_pdu {
  rnti_t                   rnti;
  uint32_t                 handle = 0;
  crb_interval             bwp;
  subcarrier_spacing       scs;
  cyclic_prefix            cp;
  pucch_format             format_type;
  pucch_repetition_tx_slot multi_slot_tx_indicator;
  bool                     pi2_bpsk;
  prb_interval             prbs;
  ofdm_symbol_range        symbols;
  bool                     intra_slot_frequency_hopping;
  uint16_t                 second_hop_prb;
  pucch_group_hopping      pucch_grp_hopping;
  uint8_t                  reserved;
  uint16_t                 nid_pucch_hopping;
  uint16_t                 initial_cyclic_shift;
  uint16_t                 nid_pucch_scrambling;
  uint8_t                  time_domain_occ_index;
  uint8_t                  pre_dft_occ_idx;
  uint8_t                  pre_dft_occ_len;
  bool                     add_dmrs_flag;
  uint16_t                 nid0_pucch_dmrs_scrambling;
  uint8_t                  m0_pucch_dmrs_cyclic_shift;
  uint8_t                  sr_bit_len;
  uint16_t                 bit_len_harq;
  uint16_t                 csi_part1_bit_length;
  //: TODO: beamforming struct
  ul_pucch_maintenance_v3              pucch_maintenance_v3;
  uci_part1_to_part2_correspondence_v3 uci_correspondence;
};

} // namespace fapi
} // namespace ocudu
