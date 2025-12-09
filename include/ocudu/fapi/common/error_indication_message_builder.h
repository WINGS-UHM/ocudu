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

#include "ocudu/fapi/common/error_indication.h"
#include <limits>

namespace ocudu {
namespace fapi {

/// \brief Builds and returns an ERROR.indication message with the given parameters, as per SCF-222 v4.0 section 3.3.6.1
/// in table ERROR.indication message body
/// \note This builder is used to build any error code id but OUT_OF_SYNC error.
inline error_indication_message
build_error_indication(uint16_t sfn, uint16_t slot, message_type_id msg_id, error_code_id error_id)
{
  error_indication_message msg;

  msg.message_type  = message_type_id::error_indication;
  msg.sfn           = sfn;
  msg.slot          = slot;
  msg.message_id    = msg_id;
  msg.error_code    = error_id;
  msg.expected_sfn  = std::numeric_limits<decltype(error_indication_message::expected_sfn)>::max();
  msg.expected_slot = std::numeric_limits<decltype(error_indication_message::expected_slot)>::max();

  return msg;
}

/// \brief Builds and returns an ERROR.indication message with the given parameters, as per SCF-222 v4.0 section 3.3.6.1
/// in table ERROR.indication message body
/// \note This builder is used to build only an OUT_OF_SYNC error code.
inline error_indication_message build_out_of_sync_error_indication(uint16_t        sfn,
                                                                   uint16_t        slot,
                                                                   message_type_id msg_id,
                                                                   uint16_t        expected_sfn,
                                                                   uint16_t        expected_slot)
{
  error_indication_message msg;

  msg.message_type  = message_type_id::error_indication;
  msg.sfn           = sfn;
  msg.slot          = slot;
  msg.message_id    = msg_id;
  msg.error_code    = error_code_id::out_of_sync;
  msg.expected_sfn  = expected_sfn;
  msg.expected_slot = expected_slot;

  return msg;
}

/// \brief Builds and returns an ERROR.indication message with the given parameters, as per SCF-222 v4.0 section 3.3.6.1
/// in table ERROR.indication message body
/// \note This builder is used to build only an MSG_INVALID_SFN error code.
inline error_indication_message build_invalid_sfn_error_indication(uint16_t        sfn,
                                                                   uint16_t        slot,
                                                                   message_type_id msg_id,
                                                                   uint16_t        expected_sfn,
                                                                   uint16_t        expected_slot)
{
  error_indication_message msg;

  msg.message_type  = message_type_id::error_indication;
  msg.sfn           = sfn;
  msg.slot          = slot;
  msg.message_id    = msg_id;
  msg.error_code    = error_code_id::msg_invalid_sfn;
  msg.expected_sfn  = expected_sfn;
  msg.expected_slot = expected_slot;

  return msg;
}

/// \brief Builds and returns an ERROR.indication message with the given parameters, as per SCF-222 v4.0 section 3.3.6.1
/// in table ERROR.indication message body
/// \note This builder is used to build only a MSG_SLOT_ERR error code.
inline error_indication_message build_msg_error_indication(uint16_t sfn, uint16_t slot, message_type_id msg_id)
{
  error_indication_message msg;

  msg.message_type  = message_type_id::error_indication;
  msg.sfn           = sfn;
  msg.slot          = slot;
  msg.message_id    = msg_id;
  msg.error_code    = error_code_id::msg_slot_err;
  msg.expected_sfn  = std::numeric_limits<decltype(error_indication_message::expected_sfn)>::max();
  msg.expected_slot = std::numeric_limits<decltype(error_indication_message::expected_slot)>::max();

  return msg;
}

/// \brief Builds and returns an ERROR.indication message with the given parameters, as per SCF-222 v4.0 section 3.3.6.1
/// in table ERROR.indication message body
/// \note This builder is used to build only a MSG_TX_ERR error code.
inline error_indication_message build_msg_tx_error_indication(uint16_t sfn, uint16_t slot)
{
  error_indication_message msg;

  msg.message_type  = message_type_id::error_indication;
  msg.sfn           = sfn;
  msg.slot          = slot;
  msg.message_id    = message_type_id::tx_data_request;
  msg.error_code    = error_code_id::msg_tx_err;
  msg.expected_sfn  = std::numeric_limits<decltype(error_indication_message::expected_sfn)>::max();
  msg.expected_slot = std::numeric_limits<decltype(error_indication_message::expected_slot)>::max();

  return msg;
}

/// \brief Builds and returns an ERROR.indication message with the given parameters, as per SCF-222 v4.0 section 3.3.6.1
/// in table ERROR.indication message body
/// \note This builder is used to build only a MSG_UL_DCI_ERR error code.
inline error_indication_message build_msg_ul_dci_error_indication(uint16_t sfn, uint16_t slot)
{
  error_indication_message msg;

  msg.message_type  = message_type_id::error_indication;
  msg.sfn           = sfn;
  msg.slot          = slot;
  msg.message_id    = message_type_id::ul_dci_request;
  msg.error_code    = error_code_id::msg_ul_dci_err;
  msg.expected_sfn  = std::numeric_limits<decltype(error_indication_message::expected_sfn)>::max();
  msg.expected_slot = std::numeric_limits<decltype(error_indication_message::expected_slot)>::max();

  return msg;
}

} // namespace fapi
} // namespace ocudu
