/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
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
  return {};
}
