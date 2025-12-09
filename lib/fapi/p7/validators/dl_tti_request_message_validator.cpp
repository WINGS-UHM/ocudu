/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/dl_tti_request_message_validator.h"
#include "p7_validator_helpers.h"
#include "pdus/dl_csi_pdu.h"
#include "pdus/dl_pdcch_pdu.h"
#include "pdus/dl_pdsch_pdu.h"
#include "pdus/dl_prs_pdu.h"
#include "pdus/dl_ssb_pdu.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"

using namespace ocudu;
using namespace fapi;

error_type<validator_report> ocudu::fapi::validate_dl_tti_request(const dl_tti_request_message& msg)
{
  validator_report report(msg.sfn, msg.slot);

  // Validate the SFN and slot.
  bool success = true;
  success &= validate_sfn(msg.sfn, message_type_id::dl_tti_request, report);
  success &= validate_slot(msg.slot, message_type_id::dl_tti_request, report);
  success &= validate_num_groups(msg.num_groups, message_type_id::dl_tti_request, report);

  // Validate each PDU.
  for (const auto& pdu : msg.pdus) {
    switch (pdu.pdu_type) {
      case dl_pdu_type::SSB:
        success &= validate_dl_ssb_pdu(pdu.ssb_pdu, report);
        break;
      case dl_pdu_type::PDCCH:
        success &= validate_dl_pdcch_pdu(message_type_id::dl_tti_request, pdu.pdcch_pdu, report);
        break;
      case dl_pdu_type::PDSCH:
        success &= validate_dl_pdsch_pdu(pdu.pdsch_pdu, report);
        break;
      case dl_pdu_type::CSI_RS:
        success &= validate_dl_csi_pdu(pdu.csi_rs_pdu, report);
        break;
      case dl_pdu_type::PRS:
        success &= validate_dl_prs_pdu(pdu.prs_pdu, report);
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
