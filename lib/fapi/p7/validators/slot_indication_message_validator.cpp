/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/validators/slot_indication_message_validator.h"
#include "validator_helpers.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"

using namespace ocudu;
using namespace fapi;

error_type<validator_report> ocudu::fapi::validate_slot_indication(const slot_indication& msg)
{
  validator_report report(msg.sfn, msg.slot);

  static constexpr message_type_id msg_id = message_type_id::slot_indication;

  // Validate the SFN and slot.
  bool success = true;
  success &= validate_sfn(msg.sfn, msg_id, report);
  success &= validate_slot(msg.slot, msg_id, report);

  // Build the result.
  if (!success) {
    return make_unexpected(std::move(report));
  }

  return {};
}
