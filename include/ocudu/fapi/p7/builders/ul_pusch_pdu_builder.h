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
#include "ocudu/fapi/p7/messages/ul_pusch_pdu.h"
#include "ocudu/ran/dmrs/dmrs.h"
#include "ocudu/ran/ptrs/ptrs.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// PUSCH PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.3.2.
class ul_pusch_pdu_builder
{
  ul_pusch_pdu& pdu;

public:
  explicit ul_pusch_pdu_builder(ul_pusch_pdu& pdu_) : pdu(pdu_)
  {
    pdu.ul_dmrs_symb_pos = 0U;
    pdu.rb_bitmap.fill(0);

    ul_pusch_uci& uci        = pdu.pusch_uci;
    uci.harq_ack_bit_length  = 0U;
    uci.beta_offset_harq_ack = 0U;
    uci.csi_part1_bit_length = 0U;
    uci.beta_offset_csi1     = 0U;
    uci.flags_csi_part2      = 0U;
    uci.beta_offset_csi2     = 0U;
  }

  /// Sets the PUSCH PDU UE specific parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_ue_specific_parameters(rnti_t rnti)
  {
    pdu.rnti = rnti;

    return *this;
  }

  /// Sets the PUSCH PDU BWP parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_bwp_parameters(crb_interval bwp, subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.bwp = bwp;
    pdu.scs = scs;
    pdu.cp  = cp;

    return *this;
  }

  /// Sets the PUSCH PDU information parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_information_parameters(float             target_code_rate,
                                                   modulation_scheme qam_mod_order,
                                                   uint8_t           mcs_index,
                                                   pusch_mcs_table   mcs_table,
                                                   bool              transform_precoding,
                                                   uint16_t          nid_pusch,
                                                   uint8_t           num_layers)
  {
    pdu.target_code_rate    = target_code_rate * 10.F;
    pdu.qam_mod_order       = qam_mod_order;
    pdu.mcs_index           = mcs_index;
    pdu.mcs_table           = mcs_table;
    pdu.transform_precoding = transform_precoding;
    pdu.nid_pusch           = nid_pusch;
    pdu.num_layers          = num_layers;

    return *this;
  }

  /// Sets the PUSCH PDU DMRS parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_dmrs_parameters(uint16_t           ul_dmrs_symb_pos,
                                            dmrs_config_type   dmrs_type,
                                            uint16_t           pusch_dmrs_scrambling_id,
                                            uint16_t           pusch_dmrs_scrambling_id_complement,
                                            low_papr_dmrs_type low_papr_dmrs,
                                            uint16_t           pusch_dmrs_identity,
                                            uint8_t            nscid,
                                            uint8_t            num_dmrs_cdm_grps_no_data,
                                            uint16_t           dmrs_ports)
  {
    pdu.ul_dmrs_symb_pos = ul_dmrs_symb_pos;
    pdu.dmrs_type        = (dmrs_type == dmrs_config_type::type1) ? dmrs_cfg_type::type_1 : dmrs_cfg_type::type_2;
    pdu.pusch_dmrs_scrambling_id            = pusch_dmrs_scrambling_id;
    pdu.pusch_dmrs_scrambling_id_complement = pusch_dmrs_scrambling_id_complement;
    pdu.low_papr_dmrs                       = low_papr_dmrs;
    pdu.pusch_dmrs_identity                 = pusch_dmrs_identity;
    pdu.nscid                               = nscid;
    pdu.num_dmrs_cdm_grps_no_data           = num_dmrs_cdm_grps_no_data;
    pdu.dmrs_ports                          = dmrs_ports;

    return *this;
  }

  /// Sets the PUSCH PDU allocation in frequency domain type 0 parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_allocation_in_frequency_type_0_parameters(span<const uint8_t> rb_bitmap,
                                                                      uint16_t            tx_direct_current_location,
                                                                      bool                uplink_frequency_shift_7p5hHz)
  {
    pdu.resource_alloc = resource_allocation_type::type_0;
    ocudu_assert(pdu.rb_bitmap.size() == rb_bitmap.size(), "RB bitmap size doesn't match");
    std::copy(rb_bitmap.begin(), rb_bitmap.end(), pdu.rb_bitmap.begin());
    pdu.vrb_to_prb_mapping            = vrb_to_prb_mapping_type::non_interleaved;
    pdu.tx_direct_current_location    = tx_direct_current_location;
    pdu.uplink_frequency_shift_7p5kHz = uplink_frequency_shift_7p5hHz;

    // Set the VRBs parameters for type 1 to a value.
    pdu.vrbs                         = {0, 0};
    pdu.intra_slot_frequency_hopping = false;

    return *this;
  }

  /// Sets the PUSCH PDU allocation in frequency domain type 1 parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_allocation_in_frequency_type_1_parameters(vrb_interval vrbs,
                                                                      bool         intra_slot_frequency_hopping,
                                                                      uint16_t     tx_direct_current_location,
                                                                      bool         uplink_frequency_shift_7p5hHz)
  {
    pdu.resource_alloc                = resource_allocation_type::type_1;
    pdu.vrbs                          = vrbs;
    pdu.intra_slot_frequency_hopping  = intra_slot_frequency_hopping;
    pdu.vrb_to_prb_mapping            = vrb_to_prb_mapping_type::non_interleaved;
    pdu.tx_direct_current_location    = tx_direct_current_location;
    pdu.uplink_frequency_shift_7p5kHz = uplink_frequency_shift_7p5hHz;

    return *this;
  }

  /// Sets the PUSCH PDU allocation in time domain type 0 parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder& set_allocation_in_time_parameters(ofdm_symbol_range symbols)
  {
    pdu.symbols = symbols;

    return *this;
  }

  /// Sets the PUSCH PDU maintenance v3 BWP parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH maintenance FAPIv3.
  ul_pusch_pdu_builder& set_maintenance_v3_bwp_parameters(uint8_t  pusch_trans_type,
                                                          uint16_t delta_bwp0_start_from_active_bwp,
                                                          uint16_t initial_ul_bwp_size)
  {
    auto& v3                            = pdu.pusch_maintenance_v3;
    v3.pusch_trans_type                 = pusch_trans_type;
    v3.delta_bwp0_start_from_active_bwp = delta_bwp0_start_from_active_bwp;
    v3.initial_ul_bwp_size              = initial_ul_bwp_size;

    return *this;
  }

  /// Sets the PUSCH PDU maintenance v3 DMRS parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH maintenance FAPIv3.
  ul_pusch_pdu_builder& set_maintenance_v3_dmrs_parameters(uint8_t group_or_sequence_hopping)
  {
    pdu.pusch_maintenance_v3.group_or_sequence_hopping = group_or_sequence_hopping;

    return *this;
  }

  /// Sets the PUSCH PDU maintenance v3 frequency domain allocation parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH maintenance FAPIv3.
  ul_pusch_pdu_builder& set_maintenance_v3_frequency_allocation_parameters(uint16_t             pusch_second_hop_prb,
                                                                           ldpc_base_graph_type ldpc_graph,
                                                                           units::bytes         tb_size_lbrm_bytes)
  {
    auto& v3                = pdu.pusch_maintenance_v3;
    v3.pusch_second_hop_prb = pusch_second_hop_prb;
    v3.ldpc_base_graph      = ldpc_graph;
    v3.tb_size_lbrm_bytes   = tb_size_lbrm_bytes;

    return *this;
  }

  /// Sets the PUSCH PDU parameters v4 basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH parameters v4.
  ul_pusch_pdu_builder& set_parameters_v4_basic_parameters(bool cb_crc_status_request)
  {
    pdu.pusch_params_v4.cb_crc_status_request = cb_crc_status_request;

    return *this;
  }

  /// Adds a UCI part1 to part2 correspondence v3 to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table UCI information for determining UCI
  /// Part1 to PArt2 correspondence, added in FAPIv3.
  ul_pusch_pdu_builder&
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

  // :TODO: UL MIMO parameters in FAPIv4.

  /// Adds optional PUSCH data information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH data information.
  // :TODO: analyze in the future this function. I'd suggest to change the last 2 arguments with a bounded_bitset or a
  // vector of bool.
  ul_pusch_pdu_builder& add_optional_pusch_data(uint8_t             rv_index,
                                                uint8_t             harq_process_id,
                                                bool                new_data,
                                                units::bytes        tb_size,
                                                uint16_t            num_cb,
                                                span<const uint8_t> cb_present_and_position)
  {
    pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_DATA_BIT);

    auto& data           = pdu.pusch_data;
    data.rv_index        = rv_index;
    data.harq_process_id = harq_process_id;
    data.new_data        = new_data;
    data.tb_size         = tb_size;
    data.num_cb          = num_cb;
    data.cb_present_and_position.assign(cb_present_and_position.begin(), cb_present_and_position.end());

    return *this;
  }

  /// Adds optional PUSCH UCI alpha information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH UCI information.
  ul_pusch_pdu_builder& add_optional_pusch_uci_alpha(alpha_scaling_opt alpha_scaling)
  {
    pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_UCI_BIT);

    auto& uci         = pdu.pusch_uci;
    uci.alpha_scaling = alpha_scaling;

    return *this;
  }

  /// Adds optional PUSCH UCI HARQ information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH UCI information.
  ul_pusch_pdu_builder& add_optional_pusch_uci_harq(uint16_t harq_ack_bit_len, uint8_t beta_offset_harq_ack)
  {
    pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_UCI_BIT);

    auto& uci = pdu.pusch_uci;

    uci.harq_ack_bit_length  = harq_ack_bit_len;
    uci.beta_offset_harq_ack = beta_offset_harq_ack;

    return *this;
  }

  /// Adds optional PUSCH UCI CSI1 information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH UCI information.
  ul_pusch_pdu_builder& add_optional_pusch_uci_csi1(uint16_t csi_part1_bit_len, uint8_t beta_offset_csi_1)
  {
    pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_UCI_BIT);

    auto& uci = pdu.pusch_uci;

    uci.csi_part1_bit_length = csi_part1_bit_len;
    uci.beta_offset_csi1     = beta_offset_csi_1;

    return *this;
  }

  /// Adds optional PUSCH UCI CSI2 information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH UCI information.
  ul_pusch_pdu_builder& add_optional_pusch_uci_csi2(uint8_t beta_offset_csi_2)
  {
    auto& uci = pdu.pusch_uci;

    uci.flags_csi_part2  = std::numeric_limits<decltype(fapi::ul_pusch_uci::flags_csi_part2)>::max();
    uci.beta_offset_csi2 = beta_offset_csi_2;

    return *this;
  }

  /// Adds optional PUSCH PTRS information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH PTRS information.
  ul_pusch_pdu_builder& add_optional_pusch_ptrs(span<const ul_pusch_ptrs::ptrs_port_info> port_info,
                                                ptrs_time_density                         ptrs_time_density,
                                                ptrs_frequency_density                    ptrs_freq_density,
                                                ul_ptrs_power_type                        ul_ptrs_power)
  {
    pdu.pdu_bitmap.set(ul_pusch_pdu::PUSCH_PTRS_BIT);

    auto& ptrs = pdu.pusch_ptrs;

    ptrs.port_info.assign(port_info.begin(), port_info.end());
    ptrs.ul_ptrs_power     = ul_ptrs_power;
    ptrs.ptrs_time_density = static_cast<uint8_t>(ptrs_time_density) / 2U;
    ptrs.ptrs_freq_density = static_cast<uint8_t>(ptrs_freq_density) / 4U;

    return *this;
  }

  /// Adds optional PUSCH DFTS OFDM information to the PUSCH PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table optional PUSCH DFTS OFDM
  /// information.
  ul_pusch_pdu_builder& add_optional_dfts_ofdm(uint8_t  low_papr_group_number,
                                               uint16_t low_papr_sequence_number,
                                               uint8_t  ul_ptrs_sample_density,
                                               uint8_t  ul_ptrs_time_density_transform_precoding)
  {
    pdu.pdu_bitmap.set(ul_pusch_pdu::DFTS_OFDM_BIT);

    auto& ofdm = pdu.pusch_ofdm;

    ofdm.low_papr_group_number                    = low_papr_group_number;
    ofdm.low_papr_sequence_number                 = low_papr_sequence_number;
    ofdm.ul_ptrs_sample_density                   = ul_ptrs_sample_density;
    ofdm.ul_ptrs_time_density_transform_precoding = ul_ptrs_time_density_transform_precoding;

    return *this;
  }

  /// Sets the PUSCH context as vendor specific.
  ul_pusch_pdu_builder& set_context_vendor_specific(rnti_t rnti, harq_id_t harq_id)
  {
    pdu.context = pusch_context(rnti, harq_id);
    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
