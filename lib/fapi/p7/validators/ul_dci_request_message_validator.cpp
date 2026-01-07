/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/ul_dci_request_message_validator.h"
#include "pdus/dl_pdcch_pdu.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/ul_dci_request.h"

using namespace ocudu;
using namespace fapi;

error_type<validator_report> ocudu::fapi::validate_ul_dci_request(const ul_dci_request& msg)
{
  validator_report report(msg.sfn, msg.slot);

  // Validate the SFN and slot.
  bool success = true;
  success &= validate_sfn(msg.sfn, message_type_id::ul_dci_request, report);
  success &= validate_slot(msg.slot, message_type_id::ul_dci_request, report);

  // Validate each PDU.
  for (const auto& pdu : msg.pdus) {
    switch (pdu.pdu_type) {
      case ul_dci_pdu_type::PDCCH:
        success &= validate_dl_pdcch_pdu(message_type_id::ul_dci_request, pdu.pdu, report);
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
