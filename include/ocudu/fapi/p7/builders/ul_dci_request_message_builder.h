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

#include "ocudu/fapi/p7/builders/dl_pdcch_pdu_builder.h"
#include "ocudu/fapi/p7/messages/ul_dci_request.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// UL_DCI.request message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.4.
class ul_dci_request_message_builder
{
  ul_dci_request_message& msg;

public:
  explicit ul_dci_request_message_builder(ul_dci_request_message& msg_) : msg(msg_)
  {
    msg.num_pdus_of_each_type.fill(0);
  }

  /// Sets the UL_DCI.request basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.4 in table UL_DCI.request message body.
  ul_dci_request_message_builder& set_basic_parameters(uint16_t sfn, uint16_t slot)
  {
    msg.sfn  = sfn;
    msg.slot = slot;

    return *this;
  }

  /// Adds a PDCCH PDU to the UL_DCI.request basic parameters and returns a reference to the PDCCH PDU builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.4 in table UL_DCI.request message body.
  /// \param[in] nof_dci_in_pdu Number of DCIs in the PDCCH PDU.
  dl_pdcch_pdu_builder add_pdcch_pdu(unsigned nof_dci_in_pdu)
  {
    unsigned pdcch_index = msg.pdus.size();
    auto&    pdu         = msg.pdus.emplace_back();

    // Fill the pdcch pdu index value. The index value will be the index of the pdu in the array of PDCCH pdus.
    pdu.pdu.maintenance_v3.pdcch_pdu_index = pdcch_index;

    // Increase the number of PDCCH pdus in the request.
    ++msg.num_pdus_of_each_type[static_cast<size_t>(ul_dci_pdu_type::PDCCH)];
    msg.num_pdus_of_each_type[ul_dci_request_message::DCI_INDEX] += nof_dci_in_pdu;

    pdu.pdu_type = ul_dci_pdu_type::PDCCH;

    dl_pdcch_pdu_builder builder(pdu.pdu);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
