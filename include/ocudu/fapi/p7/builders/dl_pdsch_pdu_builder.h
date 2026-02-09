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

#include "ocudu/fapi/p7/builders/message_builder_helper.h"
#include "ocudu/fapi/p7/builders/tx_precoding_and_beamforming_pdu_builder.h"
#include "ocudu/fapi/p7/messages/dl_pdsch_pdu.h"
#include "ocudu/fapi/p7/messages/power_control_offset_ss.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/dmrs/dmrs.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// Builder that helps to fill the parameters of a DL PDSCH codeword.
class dl_pdsch_codeword_builder
{
public:
  dl_pdsch_codeword_builder(dl_pdsch_codeword& cw_, uint8_t& cbg_tx_information_) :
    cw(cw_), cbg_tx_information(cbg_tx_information_)
  {
  }

  /// Sets the codeword basic parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_codeword_builder& set_basic_parameters(float        target_code_rate,
                                                  uint8_t      qam_mod,
                                                  uint8_t      mcs_index,
                                                  uint8_t      mcs_table,
                                                  uint8_t      rv_index,
                                                  units::bytes tb_size)
  {
    cw.target_code_rate = target_code_rate * 10.F;
    cw.qam_mod_order    = qam_mod;
    cw.mcs_index        = mcs_index;
    cw.mcs_table        = mcs_table;
    cw.rv_index         = rv_index;
    cw.tb_size          = tb_size;

    return *this;
  }

  /// Sets the maintenance v3 parameters of the codeword.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters V3.
  dl_pdsch_codeword_builder& set_maintenance_v3_parameters(uint8_t cbg_tx_info)
  {
    cbg_tx_information = cbg_tx_info;

    return *this;
  }

private:
  dl_pdsch_codeword& cw;
  uint8_t&           cbg_tx_information;
};

/// DL PDSCH PDU builder that helps to fill the parameters specified in SCF-222 v4.0 section 3.4.2.2.
class dl_pdsch_pdu_builder
{
public:
  explicit dl_pdsch_pdu_builder(dl_pdsch_pdu& pdu_) : pdu(pdu_)
  {
    pdu.pdu_bitmap                           = 0U;
    pdu.is_last_cb_present                   = 0U;
    pdu.pdsch_maintenance_v3.tb_crc_required = 0U;
    pdu.rb_bitmap.fill(0);
    pdu.dl_tb_crc_cw.fill(0);
    pdu.pdsch_maintenance_v3.ssb_pdus_for_rate_matching.fill(0);
  }

  /// Returns the PDU index.
  unsigned get_pdu_id() const { return pdu.pdu_index; }

  /// Sets the basic parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_basic_parameters(rnti_t rnti)
  {
    pdu.rnti = rnti;

    return *this;
  }

  /// Sets the BWP parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_bwp_parameters(crb_interval bwp, subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.bwp = bwp;
    pdu.scs = scs;
    pdu.cp  = cp;

    return *this;
  }

  /// Adds a codeword to the PDSCH PDU and returns a codeword builder to fill the codeword parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_codeword_builder add_codeword()
  {
    dl_pdsch_codeword_builder builder(pdu.cws.emplace_back(),
                                      pdu.pdsch_maintenance_v3.cbg_tx_information.emplace_back());

    return builder;
  }

  /// Sets the codeword information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_codeword_information_parameters(uint16_t             n_id_pdsch,
                                                            uint8_t              num_layers,
                                                            uint8_t              trasnmission_scheme,
                                                            pdsch_ref_point_type ref_point)
  {
    pdu.nid_pdsch           = n_id_pdsch;
    pdu.num_layers          = num_layers;
    pdu.transmission_scheme = trasnmission_scheme;
    pdu.ref_point           = ref_point;

    return *this;
  }

  /// Sets the DMRS parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_dmrs_parameters(uint16_t           dl_dmrs_symb_pos,
                                            dmrs_config_type   dmrs_type,
                                            uint16_t           pdsch_dmrs_scrambling_id,
                                            uint16_t           pdsch_dmrs_scrambling_id_complement,
                                            low_papr_dmrs_type low_papr_dmrs,
                                            uint8_t            nscid,
                                            uint8_t            num_dmrs_cdm_groups_no_data,
                                            uint16_t           dmrs_ports)
  {
    pdu.dl_dmrs_symb_pos = dl_dmrs_symb_pos;
    pdu.dmrs_type        = (dmrs_type == dmrs_config_type::type1) ? dmrs_cfg_type::type_1 : dmrs_cfg_type::type_2;
    pdu.pdsch_dmrs_scrambling_id       = pdsch_dmrs_scrambling_id;
    pdu.pdsch_dmrs_scrambling_id_compl = pdsch_dmrs_scrambling_id_complement;
    pdu.low_papr_dmrs                  = low_papr_dmrs;
    pdu.nscid                          = nscid;
    pdu.num_dmrs_cdm_grps_no_data      = num_dmrs_cdm_groups_no_data;
    pdu.dmrs_ports                     = dmrs_ports;

    return *this;
  }

  /// Sets the PDSCH allocation in frequency type 0 parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_frequency_type_0(span<const uint8_t>     rb_map,
                                                                 vrb_to_prb_mapping_type vrb_to_prb_mapping)
  {
    pdu.resource_alloc     = resource_allocation_type::type_0;
    pdu.vrb_to_prb_mapping = vrb_to_prb_mapping;

    ocudu_assert(rb_map.size() <= dl_pdsch_pdu::MAX_SIZE_RB_BITMAP,
                 "[PDSCH Builder] - Incoming RB bitmap size {} exceeds FAPI bitmap field {}",
                 rb_map.size(),
                 int(dl_pdsch_pdu::MAX_SIZE_RB_BITMAP));

    std::copy(rb_map.begin(), rb_map.end(), pdu.rb_bitmap.begin());

    // Fill in the VRBS fields, although they belong to allocation type 1.
    pdu.vrbs = {0, 0};

    return *this;
  }

  /// Sets the PDSCH allocation in frequency type 1 parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_frequency_type_1(vrb_interval            vrbs,
                                                                 vrb_to_prb_mapping_type vrb_to_prb_mapping)
  {
    pdu.resource_alloc     = resource_allocation_type::type_1;
    pdu.vrbs               = vrbs;
    pdu.vrb_to_prb_mapping = vrb_to_prb_mapping;

    return *this;
  }

  /// Sets the PDSCH allocation in time parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_pdsch_allocation_in_time_parameters(ofdm_symbol_range symbols)
  {
    pdu.symbols = symbols;

    return *this;
  }

  dl_pdsch_pdu_builder& set_ptrs_params()
  {
    pdu.pdu_bitmap.set(dl_pdsch_pdu::PDU_BITMAP_PTRS_BIT);
    // :TODO: Implement me!

    return *this;
  }

  // :TODO: Beamforming.

  /// Sets the profile NR Tx Power info parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_profile_nr_tx_power_info_parameters(int                     power_control_offset,
                                                                power_control_offset_ss power_control_offset_ss)
  {
    auto& power = pdu.power_config.emplace<dl_pdsch_pdu::power_profile_nr>();

    power.power_control_offset_profile_nr    = power_control_offset;
    power.power_control_offset_ss_profile_nr = power_control_offset_ss;

    return *this;
  }

  /// Sets the profile SSS Tx Power info parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_profile_sss_tx_power_info_parameters(float dmrs_offset_db, float data_offset_db)
  {
    auto& power                    = pdu.power_config.emplace<dl_pdsch_pdu::power_profile_sss>();
    power.data_power_offset_sss_db = data_offset_db;
    power.dmrs_power_offset_sss_db = dmrs_offset_db;

    return *this;
  }

  /// Sets the CBG ReTx Ctrl parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH PDU.
  dl_pdsch_pdu_builder& set_cbg_re_tx_ctrl_parameters(bool                 last_cb_present_first_tb,
                                                      bool                 last_cb_present_second_tb,
                                                      inline_tb_crc_type   tb_crc,
                                                      span<const uint32_t> dl_tb_crc_cw)
  {
    pdu.pdu_bitmap.set(dl_pdsch_pdu::PDU_BITMAP_CBG_RETX_CTRL_BIT);

    detail::set_bitmap_bit<uint8_t>(
        pdu.is_last_cb_present, dl_pdsch_pdu::LAST_CB_BITMAP_FIRST_TB_BIT, last_cb_present_first_tb);
    detail::set_bitmap_bit<uint8_t>(
        pdu.is_last_cb_present, dl_pdsch_pdu::LAST_CB_BITMAP_SECOND_TB_BIT, last_cb_present_second_tb);

    pdu.is_inline_tb_crc = tb_crc;

    ocudu_assert(dl_tb_crc_cw.size() <= dl_pdsch_pdu::MAX_SIZE_DL_TB_CRC,
                 "[PDSCH Builder] - Incoming DL TB CRC size ({}) out of bounds ({})",
                 dl_tb_crc_cw.size(),
                 int(dl_pdsch_pdu::MAX_SIZE_DL_TB_CRC));
    std::copy(dl_tb_crc_cw.begin(), dl_tb_crc_cw.end(), pdu.dl_tb_crc_cw.begin());

    return *this;
  }

  /// Sets the maintenance v3 BWP information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_bwp_parameters(pdsch_trans_type trans_type,
                                                          uint16_t         coreset_start_point,
                                                          uint16_t         initial_dl_bwp_size)
  {
    pdu.pdsch_maintenance_v3.trans_type          = trans_type;
    pdu.pdsch_maintenance_v3.coreset_start_point = coreset_start_point;
    pdu.pdsch_maintenance_v3.initial_dl_bwp_size = initial_dl_bwp_size;

    return *this;
  }

  /// Sets the maintenance v3 codeword information parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_codeword_parameters(ldpc_base_graph_type ldpc_base_graph,
                                                               units::bytes         tb_size_lbrm_bytes,
                                                               bool                 tb_crc_first_tb_required,
                                                               bool                 tb_crc_second_tb_required)
  {
    pdu.pdsch_maintenance_v3.ldpc_base_graph    = ldpc_base_graph;
    pdu.pdsch_maintenance_v3.tb_size_lbrm_bytes = tb_size_lbrm_bytes;

    // Fill the bitmap.
    detail::set_bitmap_bit<uint8_t>(pdu.pdsch_maintenance_v3.tb_crc_required,
                                    dl_pdsch_maintenance_parameters_v3::TB_BITMAP_FIRST_TB_BIT,
                                    tb_crc_first_tb_required);
    detail::set_bitmap_bit<uint8_t>(pdu.pdsch_maintenance_v3.tb_crc_required,
                                    dl_pdsch_maintenance_parameters_v3::TB_BITMAP_SECOND_TB_BIT,
                                    tb_crc_second_tb_required);

    return *this;
  }

  /// Sets the maintenance v3 CSI-RS rate matching references parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_csi_rm_references(span<const uint16_t> csi_rs_for_rm)
  {
    pdu.pdsch_maintenance_v3.csi_for_rm.assign(csi_rs_for_rm.begin(), csi_rs_for_rm.end());

    return *this;
  }

  /// Sets the maintenance v3 rate matching references parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder&
  set_maintenance_v3_rm_references_parameters(span<const uint16_t> ssb_pdus_for_rm,
                                              uint16_t             ssb_config_for_rm,
                                              uint8_t              prb_sym_rm_pattern_bitmap_size,
                                              span<const uint8_t>  prb_sym_rm_pattern_bitmap_by_reference,
                                              uint16_t             pdcch_pdu_index,
                                              uint16_t             dci_index,
                                              uint8_t              lte_crs_rm_pattern_bitmap_size,
                                              span<const uint8_t>  lte_crs_rm_pattern)
  {
    ocudu_assert(ssb_pdus_for_rm.size() <= dl_pdsch_maintenance_parameters_v3::MAX_SIZE_SSB_PDU_FOR_RM,
                 "[PDSCH Builder] - Incoming SSB PDUs for RM matching size ({}) doesn't fit the field ({})",
                 ssb_pdus_for_rm.size(),
                 int(dl_pdsch_maintenance_parameters_v3::MAX_SIZE_SSB_PDU_FOR_RM));
    std::copy(
        ssb_pdus_for_rm.begin(), ssb_pdus_for_rm.end(), pdu.pdsch_maintenance_v3.ssb_pdus_for_rate_matching.begin());

    pdu.pdsch_maintenance_v3.ssb_config_for_rate_matching         = ssb_config_for_rm;
    pdu.pdsch_maintenance_v3.prb_sym_rm_pattern_bitmap_size_byref = prb_sym_rm_pattern_bitmap_size;
    pdu.pdsch_maintenance_v3.prb_sym_rm_patt_bmp_byref.assign(prb_sym_rm_pattern_bitmap_by_reference.begin(),
                                                              prb_sym_rm_pattern_bitmap_by_reference.end());

    // These two parameters are set to 0 for this release FAPI v4.
    pdu.pdsch_maintenance_v3.num_prb_sym_rm_patts_by_value = 0U;
    pdu.pdsch_maintenance_v3.num_coreset_rm_patterns       = 0U;

    pdu.pdsch_maintenance_v3.pdcch_pdu_index = pdcch_pdu_index;
    pdu.pdsch_maintenance_v3.dci_index       = dci_index;

    pdu.pdsch_maintenance_v3.lte_crs_rm_pattern_bitmap_size = lte_crs_rm_pattern_bitmap_size;
    pdu.pdsch_maintenance_v3.lte_crs_rm_pattern.assign(lte_crs_rm_pattern.begin(), lte_crs_rm_pattern.end());

    return *this;
  }

  /// Sets the maintenance v3 CBG retx control parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance parameters v3.
  dl_pdsch_pdu_builder& set_maintenance_v3_cbg_tx_crtl_parameters(uint8_t max_num_cbg_per_tb)
  {
    pdu.pdsch_maintenance_v3.max_num_cbg_per_tb = max_num_cbg_per_tb;

    return *this;
  }

  /// Sets the PDSCH maintenance v4 basic parameters for the fields of the PDSCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.2, in table PDSCH maintenance FAPIv4.
  dl_pdsch_pdu_builder& set_maintenance_v4_basic_parameters(uint8_t coreset_rm_pattern_bitmap_by_reference_bitmap_size,
                                                            span<const uint8_t> coreset_rm_pattern_bitmap_by_reference,
                                                            uint8_t             lte_crs_mbsfn_derivation_method,
                                                            span<const uint8_t> lte_crs_mbsfn_pattern)
  {
    pdu.pdsch_parameters_v4.lte_crs_mbsfn_derivation_method       = lte_crs_mbsfn_derivation_method;
    pdu.pdsch_parameters_v4.coreset_rm_pattern_bitmap_size_by_ref = coreset_rm_pattern_bitmap_by_reference_bitmap_size;

    pdu.pdsch_parameters_v4.coreset_rm_pattern_bmp_by_ref.assign(coreset_rm_pattern_bitmap_by_reference.begin(),
                                                                 coreset_rm_pattern_bitmap_by_reference.end());

    pdu.pdsch_parameters_v4.lte_crs_mbsfn_pattern.assign(lte_crs_mbsfn_pattern.begin(), lte_crs_mbsfn_pattern.end());

    return *this;
  }

  /// Sets the PDSCH context as vendor specific.
  dl_pdsch_pdu_builder& set_context_vendor_specific(harq_id_t harq_id, unsigned k1, unsigned nof_retxs)
  {
    pdu.context = pdsch_context(harq_id, k1, nof_retxs);
    return *this;
  }

  /// Returns a transmission precoding and beamforming PDU builder of this PDSCH PDU.
  tx_precoding_and_beamforming_pdu_builder get_tx_precoding_and_beamforming_pdu_builder()
  {
    tx_precoding_and_beamforming_pdu_builder builder(pdu.precoding_and_beamforming);

    return builder;
  }

  // :TODO: FAPIv4 MU-MIMO.

private:
  dl_pdsch_pdu& pdu;
};

} // namespace fapi
} // namespace ocudu
