/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/ul_tti_request_message_validator.h"
#include "p7_validator_helpers.h"
#include "pdus/ul_prach_pdu.h"
#include "pdus/ul_pucch_pdu.h"
#include "pdus/ul_pusch_pdu.h"
#include "pdus/ul_srs_pdu.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"

using namespace ocudu;
using namespace fapi;

error_type<validator_report> ocudu::fapi::validate_ul_tti_request(const ul_tti_request& msg)
{
  validator_report                 report(msg.slot);
  static constexpr message_type_id msg_type = message_type_id::ul_tti_request;

  bool success = true;
  success &= validate_num_groups(msg.num_groups, msg_type, report);

  // Validate each PDU.
  for (const auto& pdu : msg.pdus) {
    switch (pdu.pdu_type) {
      case ul_pdu_type::PRACH:
        success &= validate_ul_prach_pdu(pdu.prach_pdu, report);
        break;
      case ul_pdu_type::PUCCH:
        success &= validate_ul_pucch_pdu(pdu.pucch_pdu, report);
        break;
      case ul_pdu_type::PUSCH:
        success &= validate_ul_pusch_pdu(pdu.pusch_pdu, report);
        break;
      case ul_pdu_type::SRS:
        success &= validate_ul_srs_pdu(pdu.srs_pdu, report);
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
