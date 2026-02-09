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

#include "ocudu/adt/span.h"
#include "ocudu/fapi/p7/messages/ul_pucch_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// PUCCH PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.3.3.
class ul_pucch_pdu_builder
{
  ul_pucch_pdu& pdu;

public:
  explicit ul_pucch_pdu_builder(ul_pucch_pdu& pdu_) : pdu(pdu_) {}

  /// Sets the PUCCH PDU UE specific parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_ue_specific_parameters(rnti_t rnti)
  {
    pdu.rnti = rnti;

    return *this;
  }

  /// Sets the PUCCH PDU common parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder&
  set_common_parameters(pucch_format format_type, pucch_repetition_tx_slot multi_slot_tx_type, bool pi2_bpsk)
  {
    pdu.format_type             = format_type;
    pdu.multi_slot_tx_indicator = multi_slot_tx_type;
    pdu.pi2_bpsk                = pi2_bpsk;

    return *this;
  }

  /// Sets the PUCCH PDU BWP parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_bwp_parameters(crb_interval bwp, subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.bwp = bwp;
    pdu.scs = scs;
    pdu.cp  = cp;

    return *this;
  }

  /// Sets the PUCCH PDU allocation in frequency parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_allocation_in_frequency_parameters(prb_interval prbs)
  {
    pdu.prbs = prbs;

    return *this;
  }

  /// Sets the PUCCH PDU allocation in time parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_allocation_in_time_parameters(ofdm_symbol_range symbols)
  {
    pdu.symbols = symbols;

    return *this;
  }

  /// Sets the PUCCH PDU hopping information parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_hopping_information_parameters(bool                intra_slot_frequency_hopping,
                                                           uint16_t            second_hop_prb,
                                                           pucch_group_hopping pucch_group_hopping,
                                                           uint16_t            nid_pucch_hopping,
                                                           uint16_t            initial_cyclic_shift)
  {
    pdu.intra_slot_frequency_hopping = intra_slot_frequency_hopping;
    pdu.second_hop_prb               = second_hop_prb;
    pdu.pucch_grp_hopping            = pucch_group_hopping;
    pdu.nid_pucch_hopping            = nid_pucch_hopping;
    pdu.initial_cyclic_shift         = initial_cyclic_shift;

    return *this;
  }

  /// Sets the PUCCH PDU hopping information parameters for PUCCH format 2 and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_hopping_information_format2_parameters(bool     intra_slot_frequency_hopping,
                                                                   uint16_t second_hop_prb)
  {
    pdu.intra_slot_frequency_hopping = intra_slot_frequency_hopping;
    pdu.second_hop_prb               = second_hop_prb;

    return *this;
  }

  /// Sets the PUCCH PDU scrambling parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_scrambling_parameters(uint16_t nid_pucch_scrambling)
  {
    pdu.nid_pucch_scrambling = nid_pucch_scrambling;

    return *this;
  }

  /// Sets the PUCCH PDU scrambling parameters for DM-RS and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_dmrs_scrambling(uint16_t nid0_pucch_dmrs_scrambling)
  {
    pdu.nid0_pucch_dmrs_scrambling = nid0_pucch_dmrs_scrambling;

    return *this;
  }

  /// Sets the PUCCH PDU format 1 specific parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_format1_parameters(uint8_t time_domain_occ_idx)
  {
    pdu.time_domain_occ_index = time_domain_occ_idx;

    return *this;
  }

  /// Sets the PUCCH PDU format 4 specific parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder& set_format4_parameters(uint8_t pre_dft_occ_idx, uint8_t pre_dft_occ_len)
  {
    pdu.pre_dft_occ_idx = pre_dft_occ_idx;
    pdu.pre_dft_occ_len = pre_dft_occ_len;

    return *this;
  }

  /// Sets the PUCCH PDU DMRS parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder&
  set_dmrs_parameters(bool add_dmrs_flag, uint16_t nid0_pucch_dmrs_scrambling, uint8_t m0_pucch_dmrs_cyclic_shift)
  {
    pdu.add_dmrs_flag              = add_dmrs_flag;
    pdu.nid0_pucch_dmrs_scrambling = nid0_pucch_dmrs_scrambling;
    pdu.m0_pucch_dmrs_cyclic_shift = m0_pucch_dmrs_cyclic_shift;

    return *this;
  }

  /// Sets the PUCCH PDU bit length for SR, HARQ and CSI Part 1 parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder&
  set_bit_length_parameters(uint8_t sr_bit_len, uint16_t bit_len_harq, uint16_t csi_part1_bit_length)
  {
    pdu.sr_bit_len           = sr_bit_len;
    pdu.bit_len_harq         = bit_len_harq;
    pdu.csi_part1_bit_length = csi_part1_bit_length;

    return *this;
  }

  /// Sets the PUCCH PDU maintenance v3 basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH basic extension for FAPIv3.
  ul_pucch_pdu_builder& set_maintenance_v3_basic_parameters(std::optional<unsigned> max_code_rate,
                                                            std::optional<unsigned> ul_bwp_id)
  {
    pdu.pucch_maintenance_v3.max_code_rate =
        (max_code_rate) ? static_cast<unsigned>(max_code_rate.value()) : std::numeric_limits<uint8_t>::max();
    pdu.pucch_maintenance_v3.ul_bwp_id =
        (ul_bwp_id) ? static_cast<unsigned>(ul_bwp_id.value()) : std::numeric_limits<uint8_t>::max();

    return *this;
  }

  /// Adds a UCI part1 to part2 correspondence v3 to the PUCCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table UCI information for determining UCI
  /// Part1 to PArt2 correspondence, added in FAPIv3.
  ul_pucch_pdu_builder&
  add_uci_part1_part2_corresnpondence_v3(uint16_t                                             priority,
                                         span<const uint16_t>                                 param_offset,
                                         span<const uint8_t>                                  param_sizes,
                                         uint16_t                                             part2_size_map_index,
                                         uci_part1_to_part2_correspondence_v3::map_scope_type part2_size_map_scope)
  {
    ocudu_assert(param_offset.size() == param_sizes.size(),
                 "Mismatching span sizes for param offset ({}) and param sizes ({})",
                 param_offset.size(),
                 param_sizes.size());

    auto& correspondence                = pdu.uci_correspondence.part2.emplace_back();
    correspondence.priority             = priority;
    correspondence.part2_size_map_index = part2_size_map_index;
    correspondence.part2_size_map_scope = part2_size_map_scope;

    correspondence.param_offsets.assign(param_offset.begin(), param_offset.end());
    correspondence.param_sizes.assign(param_sizes.begin(), param_sizes.end());

    return *this;
  }

  /// Sets the PUCCH PDU UCI part1 to part2 correspondence v3 basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH basic extension for FAPIv3.
  ul_pucch_pdu_builder& set_uci_part1_part2_corresnpondence_v3_basic_parameters(
      span<const uci_part1_to_part2_correspondence_v3::part2_info> correspondence)
  {
    pdu.uci_correspondence.part2.assign(correspondence.begin(), correspondence.end());

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
