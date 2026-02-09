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

class dl_tti_request_builder
{
public:
  /// Constructs a builder that will help to fill the given DL TTI request message.
  explicit dl_tti_request_builder(dl_tti_request& msg_) : msg(msg_) {}

  /// \brief Sets the DL_TTI.request slot point and returns a reference to the builder.
  ///
  /// nPDUs and nPDUsOfEachType properties are filled by the add_*_pdu() functions.
  /// These parameters are specified in SCF-222 v4.0 section 3.4.2 in table DL_TTI.request message body.
  dl_tti_request_builder& set_slot(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// Adds a PDCCH PDU to the message, fills its basic parameters using the given arguments and returns a PDCCH PDU
  /// builder.
  dl_pdcch_pdu_builder add_pdcch_pdu()
  {
    // Add a new pdu.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Increase the number of PDCCH pdus in the request.
    ++msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::PDCCH)];
    ++msg.num_pdus_of_each_type[dl_tti_request::DL_DCI_INDEX];

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
  dl_csi_rs_pdu_builder add_csi_rs_pdu()
  {
    // Add a new PDU.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Fill the CSI PDU index value. The index value will be the index of the PDU in the array of CSI PDUs.
    auto& num_csi_pdu = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::CSI_RS)];

    // Increase the number of CSI PDU.
    ++num_csi_pdu;

    pdu.pdu_type = dl_pdu_type::CSI_RS;

    dl_csi_rs_pdu_builder builder(pdu.csi_rs_pdu);

    return builder;
  }

  /// Adds a SSB PDU to the message and returns a SSB PDU builder.
  dl_ssb_pdu_builder add_ssb_pdu()
  {
    // Add a new PDU.
    dl_tti_request_pdu& pdu = msg.pdus.emplace_back();

    // Increase the number of SSB PDUs in the request.
    auto& num_ssb_pdu = msg.num_pdus_of_each_type[static_cast<size_t>(dl_pdu_type::SSB)];
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

    // Increase the number of SSB PDUs in the request.
    ++num_prs_pdu;

    pdu.pdu_type = dl_pdu_type::PRS;

    dl_prs_pdu_builder builder(info);

    return builder;
  }

private:
  dl_tti_request& msg;
};

} // namespace fapi
} // namespace ocudu
