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

/// UCI.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 Section 3.4.9.
class uci_indication_builder
{
  uci_indication& msg;

public:
  explicit uci_indication_builder(uci_indication& msg_) : msg(msg_) {}

  /// \brief Sets the \e UCI.indication slot point and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.9 in table UCI.indication message body.
  uci_indication_builder& set_slot(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// \brief Adds a PUSCH PDU to the \e UCI.indication message and returns a PUSCH PDU builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.9 in table UCI.indication message body.
  uci_pusch_pdu_builder add_pusch_pdu()
  {
    auto& pdu       = msg.pdus.emplace_back();
    auto& pusch_pdu = pdu.emplace<uci_pusch_pdu>();

    uci_pusch_pdu_builder builder(pusch_pdu);

    return builder;
  }

  /// \brief Adds a PUCCH Format 0 and Format 1 PDU to the \e UCI.indication message and returns a PUCCH Format 0 and
  /// Format 1 PDU builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.9 in table UCI.indication message body.
  uci_pucch_pdu_format_0_1_builder add_format_0_1_pucch_pdu()
  {
    auto& pdu            = msg.pdus.emplace_back();
    auto& format_0_1_pdu = pdu.emplace<uci_pucch_pdu_format_0_1>();

    uci_pucch_pdu_format_0_1_builder builder(format_0_1_pdu);

    return builder;
  }

  /// \brief Adds a PUCCH Format 2, Format 3 or Format 4 PDU to the \e UCI.indication message and returns a PUCCH
  /// Format 2, Format 3 and Format 4 PDU builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.9 in table UCI.indication message body.
  uci_pucch_pdu_format_2_3_4_builder add_format_2_3_4_pucch_pdu()
  {
    auto& pdu              = msg.pdus.emplace_back();
    auto& format_2_3_4_pdu = pdu.emplace<uci_pucch_pdu_format_2_3_4>();

    uci_pucch_pdu_format_2_3_4_builder builder(format_2_3_4_pdu);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
