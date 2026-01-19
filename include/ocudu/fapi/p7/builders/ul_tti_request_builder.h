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

#include "ocudu/fapi/p7/builders/ul_prach_pdu_builder.h"
#include "ocudu/fapi/p7/builders/ul_pucch_pdu_builder.h"
#include "ocudu/fapi/p7/builders/ul_pusch_pdu_builder.h"
#include "ocudu/fapi/p7/builders/ul_srs_pdu_builder.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// UL_TTI.request message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.3.
class ul_tti_request_builder
{
  ul_tti_request& msg;
  using pdu_type = ul_tti_request::pdu_type;

public:
  explicit ul_tti_request_builder(ul_tti_request& msg_) : msg(msg_) { msg.num_pdus_of_each_type.fill(0); }

  /// Sets the UL_TTI.request basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3 in table UL_TTI,request message body.
  ul_tti_request_builder& set_basic_parameters(slot_point slot)
  {
    msg.slot = slot;

    // NOTE: number of PDU groups set to 0, because groups are not enabled in this FAPI message at the moment.
    msg.num_groups = 0;

    return *this;
  }

  /// Adds a PRACH PDU to the message and returns a builder that helps to fill the parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.1 in table PRACH PDU.
  ul_prach_pdu_builder add_prach_pdu()
  {
    auto& pdu    = msg.pdus.emplace_back();
    pdu.pdu_type = ul_pdu_type::PRACH;

    ++msg.num_pdus_of_each_type[static_cast<unsigned>(pdu_type::PRACH)];

    ul_prach_pdu_builder builder(pdu.prach_pdu);

    return builder;
  }

  /// Adds a PUCCH PDU to the message with the given format type and returns a builder that helps to fill the
  /// parameters.
  ul_pucch_pdu_builder add_pucch_pdu(pucch_format format_type)
  {
    auto& pdu    = msg.pdus.emplace_back();
    pdu.pdu_type = ul_pdu_type::PUCCH;

    if (format_type == pucch_format::FORMAT_0 || format_type == pucch_format::FORMAT_1) {
      ++msg.num_pdus_of_each_type[static_cast<unsigned>(pdu_type::PUCCH_format01)];
    } else {
      ++msg.num_pdus_of_each_type[static_cast<unsigned>(pdu_type::PUCCH_format234)];
    }

    ul_pucch_pdu_builder builder(pdu.pucch_pdu);
    pdu.pucch_pdu.format_type = format_type;

    return builder;
  }

  /// Adds a PUCCH PDU to the message and returns a builder that helps to fill the parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table PUCCH PDU.
  ul_pucch_pdu_builder add_pucch_pdu(rnti_t rnti, pucch_format format_type)
  {
    ul_pucch_pdu_builder builder = add_pucch_pdu(format_type);
    builder.set_ue_specific_parameters(rnti);

    return builder;
  }

  /// Adds a PUSCH PDU to the message and returns a builder that helps to fill the parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder add_pusch_pdu()
  {
    auto& pdu    = msg.pdus.emplace_back();
    pdu.pdu_type = ul_pdu_type::PUSCH;

    ++msg.num_pdus_of_each_type[static_cast<unsigned>(pdu_type::PUSCH)];

    ul_pusch_pdu_builder builder(pdu.pusch_pdu);

    return builder;
  }

  /// Adds a PUSCH PDU to the message and returns a builder that helps to fill the parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.2 in table PUSCH PDU.
  ul_pusch_pdu_builder add_pusch_pdu(rnti_t rnti)
  {
    ul_pusch_pdu_builder builder = add_pusch_pdu();
    builder.set_ue_specific_parameters(rnti);

    return builder;
  }

  /// Adds a SRS PDU to the message and returns a builder that helps to fill the parameters.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder add_srs_pdu()
  {
    auto& pdu    = msg.pdus.emplace_back();
    pdu.pdu_type = ul_pdu_type::SRS;

    ++msg.num_pdus_of_each_type[static_cast<unsigned>(pdu_type::SRS)];

    ul_srs_pdu_builder builder(pdu.srs_pdu);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
