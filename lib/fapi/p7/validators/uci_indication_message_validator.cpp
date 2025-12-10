/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/uci_indication_message_validator.h"
#include "pdus/uci_pdus.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/uci_indication.h"

using namespace ocudu;
using namespace fapi;

error_type<validator_report> ocudu::fapi::validate_uci_indication(const uci_indication& msg)
{
  validator_report report(msg.slot);

  bool success = true;

  // Validate each PDU.
  for (const auto& pdu : msg.pdus) {
    switch (pdu.pdu_type) {
      case uci_pdu_type::PUSCH:
        success &= validate_uci_pusch_pdu(pdu.pusch_pdu, report);
        break;
      case uci_pdu_type::PUCCH_format_0_1:
        success &= validate_uci_pucch_format01_pdu(pdu.pucch_pdu_f01, report);
        break;
      case uci_pdu_type::PUCCH_format_2_3_4:
        success &= validate_uci_pucch_format234_pdu(pdu.pucch_pdu_f234, report);
        break;
      default:
        ocudu_assert(0, "Invalid pdu_type");
        break;
    }
  }

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
