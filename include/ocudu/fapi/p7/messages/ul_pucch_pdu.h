// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/pucch/pucch_uci_bits.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include "ocudu/support/units.h"
#include <optional>
#include <variant>

namespace ocudu {
namespace fapi {

/// Holds the definition of the structure for the PUCCH PDU format 0.
struct ul_pucch_pdu_format_0 {
  uint16_t    nid_pucch_hopping;
  uint16_t    initial_cyclic_shift;
  bool        sr_present;
  units::bits bit_len_harq;
};

/// Holds the definition of the structure for the PUCCH PDU format 1.
struct ul_pucch_pdu_format_1 {
  uint16_t    nid_pucch_hopping;
  uint16_t    initial_cyclic_shift;
  uint8_t     time_domain_occ_index;
  bool        sr_present;
  units::bits bit_len_harq;
};

/// Holds the definition of the structure for the PUCCH PDU format 2.
struct ul_pucch_pdu_format_2 {
  uint16_t    nid_pucch_scrambling;
  uint16_t    nid0_pucch_dmrs_scrambling;
  sr_nof_bits sr_bit_len;
  units::bits csi_part1_bit_length;
  units::bits bit_len_harq;
};

/// Holds the definition of the structure for the PUCCH PDU format 3.
struct ul_pucch_pdu_format_3 {
  bool        pi2_bpsk;
  uint16_t    nid_pucch_hopping;
  uint16_t    nid_pucch_scrambling;
  bool        add_dmrs_flag;
  uint16_t    nid0_pucch_dmrs_scrambling;
  uint8_t     m0_pucch_dmrs_cyclic_shift;
  sr_nof_bits sr_bit_len;
  units::bits csi_part1_bit_length;
  units::bits bit_len_harq;
};

/// Holds the definition of the structure for the PUCCH PDU format 4.
struct ul_pucch_pdu_format_4 {
  bool        pi2_bpsk;
  uint16_t    nid_pucch_hopping;
  uint16_t    nid_pucch_scrambling;
  uint8_t     pre_dft_occ_idx;
  uint8_t     pre_dft_occ_len;
  bool        add_dmrs_flag;
  uint16_t    nid0_pucch_dmrs_scrambling;
  uint8_t     m0_pucch_dmrs_cyclic_shift;
  sr_nof_bits sr_bit_len;
  units::bits csi_part1_bit_length;
  units::bits bit_len_harq;
};

/// Encodes PUCCH pdu.
struct ul_pucch_pdu {
  /// Holds the possible PUCCH PDU format.
  using ul_pucch_pdu_format = std::variant<std::monostate,
                                           ul_pucch_pdu_format_0,
                                           ul_pucch_pdu_format_1,
                                           ul_pucch_pdu_format_2,
                                           ul_pucch_pdu_format_3,
                                           ul_pucch_pdu_format_4>;

  rnti_t                  rnti;
  uint32_t                handle = 0;
  crb_interval            bwp;
  subcarrier_spacing      scs;
  cyclic_prefix           cp;
  ul_pucch_pdu_format     format;
  prb_interval            prbs;
  ofdm_symbol_range       symbols;
  std::optional<uint16_t> second_hop_prb;
};

} // namespace fapi
} // namespace ocudu
