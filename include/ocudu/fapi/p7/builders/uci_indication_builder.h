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

#include "ocudu/fapi/p7/builders/uci_pucch_pdu_format_0_1_builder.h"
#include "ocudu/fapi/p7/builders/uci_pucch_pdu_format_2_3_4_builder.h"
#include "ocudu/fapi/p7/builders/uci_pusch_pdu_builder.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// UCI.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 Section 3.4.9.
class uci_indication_builder
{
  uci_indication& msg;

public:
  explicit uci_indication_builder(uci_indication& msg_) : msg(msg_) {}

  /// \brief Sets the \e UCI.indication basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9 in table UCI.indication message body.
  uci_indication_builder& set_basic_parameters(uint16_t sfn, uint16_t slot)
  {
    msg.sfn  = sfn;
    msg.slot = slot;

    return *this;
  }

  /// Adds a PUSCH PDU to the \e UCI.indication message and returns a PUSCH PDU builder.
  uci_pusch_pdu_builder add_pusch_pdu(uint32_t handle, rnti_t rnti)
  {
    uci_pusch_pdu_builder builder = add_pusch_pdu();
    builder.set_basic_parameters(handle, rnti);

    return builder;
  }

  /// Adds a PUSCH PDU to the \e UCI.indication message and returns a PUSCH PDU builder.
  uci_pusch_pdu_builder add_pusch_pdu()
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.pdu_type = uci_pdu_type::PUSCH;

    uci_pusch_pdu_builder builder(pdu.pusch_pdu);

    return builder;
  }

  /// Adds a PUCCH Format 0 and Format 1 PDU to the \e UCI.indication message and returns a PUCCH Format 0 and Format 1
  /// PDU builder.
  uci_pucch_pdu_format_0_1_builder add_format_0_1_pucch_pdu(uint32_t handle, rnti_t rnti, pucch_format type)
  {
    uci_pucch_pdu_format_0_1_builder builder = add_format_0_1_pucch_pdu();
    builder.set_basic_parameters(handle, rnti, type);

    return builder;
  }

  /// Adds a PUCCH Format 0 and Format 1 PDU to the \e UCI.indication message and returns a PUCCH Format 0 and Format 1
  /// PDU builder.
  uci_pucch_pdu_format_0_1_builder add_format_0_1_pucch_pdu()
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.pdu_type = uci_pdu_type::PUCCH_format_0_1;

    uci_pucch_pdu_format_0_1_builder builder(pdu.pucch_pdu_f01);

    return builder;
  }

  /// Adds a PUCCH Format 2, Format 3 and Format 4  PDU to the \e UCI.indication message and returns a PUCCH Format 2,
  /// Format 3 and Format 4 PDU builder.
  uci_pucch_pdu_format_2_3_4_builder add_format_2_3_4_pucch_pdu(uint32_t handle, rnti_t rnti, pucch_format type)
  {
    uci_pucch_pdu_format_2_3_4_builder builder = add_format_2_3_4_pucch_pdu();
    builder.set_basic_parameters(handle, rnti, type);

    return builder;
  }

  /// Adds a PUCCH Format 2, Format 3 and Format 4  PDU to the \e UCI.indication message and returns a PUCCH Format 2,
  /// Format 3 and Format 4 PDU builder.
  uci_pucch_pdu_format_2_3_4_builder add_format_2_3_4_pucch_pdu()
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.pdu_type = uci_pdu_type::PUCCH_format_2_3_4;

    uci_pucch_pdu_format_2_3_4_builder builder(pdu.pucch_pdu_f234);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
