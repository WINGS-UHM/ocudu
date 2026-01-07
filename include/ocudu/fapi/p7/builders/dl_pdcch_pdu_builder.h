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

#include "ocudu/fapi/p7/builders/tx_precoding_and_beamforming_pdu_builder.h"
#include "ocudu/fapi/p7/messages/dl_pdcch_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// Helper class to fill in the DL DCI PDU parameters specified in SCF-222 v4.0 section 3.4.2.1, including the PDCCH PDU
/// maintenance FAPIv3 and PDCCH PDU FAPIv4 parameters.
class dl_dci_pdu_builder
{
public:
  dl_dci_pdu_builder(dl_dci_pdu&                                    pdu_,
                     dl_pdcch_pdu_maintenance_v3::maintenance_info& pdu_v3_,
                     dl_pdcch_pdu_parameters_v4::dci_params&        pdu_v4_) :
    pdu(pdu_), pdu_v3(pdu_v3_), pdu_v4(pdu_v4_)
  {
    pdu_v3.collocated_AL16_candidate = false;
  }

  /// Sets the basic parameters for the fields of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table DL DCI PDU.
  dl_dci_pdu_builder& set_basic_parameters(rnti_t   rnti,
                                           uint16_t nid_pdcch_data,
                                           uint16_t nrnti_pdcch_data,
                                           uint8_t  cce_index,
                                           uint8_t  aggregation_level)
  {
    pdu.rnti              = rnti;
    pdu.nid_pdcch_data    = nid_pdcch_data;
    pdu.nrnti_pdcch_data  = nrnti_pdcch_data;
    pdu.cce_index         = cce_index;
    pdu.aggregation_level = aggregation_level;

    return *this;
  }

  /// Sets the profile NR Tx Power info parameters for the fields of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table DL DCI PDU.
  dl_dci_pdu_builder& set_profile_nr_tx_power_info_parameters(int power_control_offset_ss_dB)
  {
    auto& power                   = pdu.power_config.emplace<dl_dci_pdu::power_profile_nr>();
    power.power_control_offset_ss = power_control_offset_ss_dB;

    return *this;
  }

  /// Sets the profile SSS Tx Power info parameters for the fields of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU maintenance PDU.
  dl_dci_pdu_builder& set_profile_sss_tx_power_info_parameters(float dmrs_offset_db, float data_offset_db)
  {
    auto& power                = pdu.power_config.emplace<dl_dci_pdu::power_profile_sss>();
    power.dmrs_power_offset_db = dmrs_offset_db;
    power.data_power_offset_db = data_offset_db;

    return *this;
  }

  /// Sets the payload of the DL DCI PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table DL DCI PDU.
  dl_dci_pdu_builder& set_payload(const dci_payload& payload)
  {
    pdu.payload = payload;

    return *this;
  }

  /// Sets the DCI parameters of the PDCCH parameters v4.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU parameters FAPIv4.
  dl_dci_pdu_builder& set_parameters_v4_dci(uint16_t nid_pdcch_dmrs)
  {
    pdu_v4.nid_pdcch_dmrs = nid_pdcch_dmrs;

    return *this;
  }

  /// Sets the PDCCH context as vendor specific.
  dl_dci_pdu_builder set_context_vendor_specific(search_space_id         ss_id,
                                                 const char*             dci_format,
                                                 std::optional<unsigned> harq_feedback_timing)
  {
    pdu.context = pdcch_context(ss_id, dci_format, harq_feedback_timing);
    return *this;
  }

  /// Returns a transmission precoding and beamforming PDU builder of this DL DCI PDU.
  tx_precoding_and_beamforming_pdu_builder get_tx_precoding_and_beamforming_pdu_builder()
  {
    tx_precoding_and_beamforming_pdu_builder builder(pdu.precoding_and_beamforming);

    return builder;
  }

private:
  dl_dci_pdu&                                    pdu;
  dl_pdcch_pdu_maintenance_v3::maintenance_info& pdu_v3;
  dl_pdcch_pdu_parameters_v4::dci_params&        pdu_v4;
};

/// Helper class to fill in the DL PDCCH PDU parameters specified in SCF-222 v4.0 section 3.4.2.1.
class dl_pdcch_pdu_builder
{
public:
  explicit dl_pdcch_pdu_builder(dl_pdcch_pdu& pdu_) : pdu(pdu_) {}

  /// Sets the BWP parameters for the fields of the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU.
  dl_pdcch_pdu_builder&
  set_bwp_parameters(uint16_t coreset_bwp_size, uint16_t coreset_bwp_start, subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.coreset_bwp_size  = coreset_bwp_size;
    pdu.coreset_bwp_start = coreset_bwp_start;
    pdu.scs               = scs;
    pdu.cp                = cp;

    return *this;
  }

  /// Sets the coreset parameters for the fields of the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU.
  dl_pdcch_pdu_builder& set_coreset_parameters(uint8_t                                          start_symbol_index,
                                               uint8_t                                          duration_symbols,
                                               const freq_resource_bitmap&                      freq_domain_resource,
                                               cce_to_reg_mapping_type                          cce_req_mapping_type,
                                               uint8_t                                          reg_bundle_size,
                                               uint8_t                                          interleaver_size,
                                               pdcch_coreset_type                               coreset_type,
                                               uint16_t                                         shift_index,
                                               coreset_configuration::precoder_granularity_type precoder_granularity)
  {
    pdu.start_symbol_index   = start_symbol_index;
    pdu.duration_symbols     = duration_symbols;
    pdu.freq_domain_resource = freq_domain_resource;
    pdu.cce_reg_mapping_type = cce_req_mapping_type;
    pdu.reg_bundle_size      = reg_bundle_size;
    pdu.interleaver_size     = interleaver_size;
    pdu.coreset_type         = coreset_type;
    pdu.shift_index          = shift_index;
    pdu.precoder_granularity = precoder_granularity;

    return *this;
  }

  /// Adds a DL DCI PDU to the PDCCH PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.1, in table PDCCH PDU.
  dl_dci_pdu_builder add_dl_dci()
  {
    // Save the size as the index value for the DL DCI.
    unsigned dci_id = pdu.dl_dci.size();

    // Set the DL DCI index.
    dl_pdcch_pdu_maintenance_v3::maintenance_info& info = pdu.maintenance_v3.info.emplace_back();
    info.dci_index                                      = dci_id;

    dl_dci_pdu_builder builder(pdu.dl_dci.emplace_back(), info, pdu.parameters_v4.params.emplace_back());

    return builder;
  }

private:
  dl_pdcch_pdu& pdu;
};

} // namespace fapi
} // namespace ocudu
