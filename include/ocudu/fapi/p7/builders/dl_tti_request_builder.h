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

#include "ocudu/fapi/p7/builders/dl_csi_rs_pdu_builder.h"
#include "ocudu/fapi/p7/builders/dl_pdcch_pdu_builder.h"
#include "ocudu/fapi/p7/builders/dl_pdsch_pdu_builder.h"
#include "ocudu/fapi/p7/builders/dl_prs_pdu_builder.h"
#include "ocudu/fapi/p7/builders/dl_ssb_pdu_builder.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

class dl_tti_request_builder
{
public:
  /// Constructs a builder that will help to fill the given DL TTI request message.
  explicit dl_tti_request_builder(dl_tti_request& msg_) : msg(msg_) {}

  /// Sets the DL_TTI.request basic parameters and returns a reference to the builder.
  /// \note nPDUs and nPDUsOfEachType properties are filled by the add_*_pdu() functions.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2 in table DL_TTI.request message body.
  dl_tti_request_builder& set_basic_parameters(uint16_t sfn, uint16_t slot, uint16_t n_groups)
  {
    msg.sfn        = sfn;
    msg.slot       = slot;
    msg.num_groups = n_groups;

    return *this;
  }

  /// Adds a PDCCH PDU to the message, fills its basic parameters using the given arguments and returns a PDCCH PDU
  /// builder.
  /// \param[in] nof_dci_in_pdu Number of DCIs in the PDCCH PDU.
  dl_pdcch_pdu_builder add_pdcch_pdu(unsigned nof_dci_in_pdu)
  {
    // Add a new pdu.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Fill the PDCCH PDU index value. The index value will be the index of the pdu in the array of PDCCH PDUs.
    dl_pdcch_pdu_maintenance_v3& info          = pdu.pdcch_pdu.maintenance_v3;
    auto&                        num_pdcch_pdu = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::PDCCH)];
    info.pdcch_pdu_index                       = num_pdcch_pdu;

    // Increase the number of PDCCH pdus in the request.
    ++num_pdcch_pdu;
    msg.num_pdus_of_each_type[dl_tti_request::DL_DCI_INDEX] += nof_dci_in_pdu;

    pdu.pdu_type = dl_pdu_type::PDCCH;

    dl_pdcch_pdu_builder builder(pdu.pdcch_pdu);

    return builder;
  }

  /// Adds a PDSCH PDU to the message, fills its basic parameters using the given arguments and returns a PDSCH PDU
  /// builder.
  dl_pdsch_pdu_builder add_pdsch_pdu(rnti_t rnti)
  {
    dl_pdsch_pdu_builder builder = add_pdsch_pdu();
    builder.set_basic_parameters(rnti);

    return builder;
  }

  /// Adds a PDSCH PDU to the message, fills its basic parameters using the given arguments and returns a PDSCH PDU
  /// builder.
  dl_pdsch_pdu_builder add_pdsch_pdu()
  {
    // Add a new PDU.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Fill the PDSCH PDU index value. The index value will be the index of the PDU in the array of PDSCH PDUs.
    auto& num_pdsch_pdu     = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::PDSCH)];
    pdu.pdsch_pdu.pdu_index = num_pdsch_pdu;

    // Increase the number of PDSCH PDU.
    ++num_pdsch_pdu;

    pdu.pdu_type = dl_pdu_type::PDSCH;

    dl_pdsch_pdu_builder builder(pdu.pdsch_pdu);

    return builder;
  }

  /// Adds a CSI-RS PDU to the message and returns a CSI-RS PDU builder.
  dl_csi_rs_pdu_builder add_csi_rs_pdu(uint16_t                         start_rb,
                                       uint16_t                         nof_rbs,
                                       csi_rs_type                      type,
                                       uint8_t                          row,
                                       const bounded_bitset<12, false>& freq_domain,
                                       uint8_t                          symb_l0,
                                       uint8_t                          symb_l1,
                                       csi_rs_cdm_type                  cdm_type,
                                       csi_rs_freq_density_type         freq_density,
                                       uint16_t                         scrambling_id)
  {
    // Add a new PDU.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Fill the CSI PDU index value. The index value will be the index of the PDU in the array of CSI PDUs.
    auto& num_csi_pdu = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::CSI_RS)];
    pdu.csi_rs_pdu.csi_rs_maintenance_v3.csi_rs_pdu_index = num_csi_pdu;

    // Increase the number of CSI PDU.
    ++num_csi_pdu;

    pdu.pdu_type = dl_pdu_type::CSI_RS;

    dl_csi_rs_pdu_builder builder(pdu.csi_rs_pdu);

    builder.set_basic_parameters(
        start_rb, nof_rbs, type, row, freq_domain, symb_l0, symb_l1, cdm_type, freq_density, scrambling_id);

    return builder;
  }

  /// Adds a SSB PDU to the message, fills its basic parameters using the given arguments and returns a SSB PDU builder.
  dl_ssb_pdu_builder add_ssb_pdu(pci_t                 phys_cell_id,
                                 beta_pss_profile_type beta_pss_profile_nr,
                                 uint8_t               ssb_block_index,
                                 uint8_t               ssb_subcarrier_offset,
                                 ssb_offset_to_pointA  ssb_offset_pointA)
  {
    dl_ssb_pdu_builder builder = add_ssb_pdu();

    // Fill the PDU basic parameters.
    builder.set_basic_parameters(
        phys_cell_id, beta_pss_profile_nr, ssb_block_index, ssb_subcarrier_offset, ssb_offset_pointA);

    return builder;
  }

  /// Adds a SSB PDU to the message and returns a SSB PDU builder.
  dl_ssb_pdu_builder add_ssb_pdu()
  {
    // Add a new PDU.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Fill the SSB PDU index value. The index value will be the index of the PDU in the array of SSB PDUs.
    dl_ssb_maintenance_v3& info        = pdu.ssb_pdu.ssb_maintenance_v3;
    auto&                  num_ssb_pdu = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::SSB)];
    info.ssb_pdu_index                 = num_ssb_pdu;

    // Increase the number of SSB PDUs in the request.
    ++num_ssb_pdu;

    pdu.pdu_type = dl_pdu_type::SSB;

    dl_ssb_pdu_builder builder(pdu.ssb_pdu);

    return builder;
  }

  /// Adds a PRS PDU to the message and returns a PRS PDU builder.
  dl_prs_pdu_builder add_prs_pdu()
  {
    // Add a new PDU.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Fill the PRS PDU index value. The index value will be the index of the PDU in the array of PRS PDUs.
    dl_prs_pdu& info        = pdu.prs_pdu;
    auto&       num_prs_pdu = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::PRS)];
    info.pdu_index          = num_prs_pdu;

    // Increase the number of SSB PDUs in the request.
    ++num_prs_pdu;

    pdu.pdu_type = dl_pdu_type::PRS;

    dl_prs_pdu_builder builder(info);

    return builder;
  }

  //: TODO: PDU groups array
  //: TODO: top level rate match patterns

private:
  dl_tti_request& msg;
};

} // namespace fapi
} // namespace ocudu
