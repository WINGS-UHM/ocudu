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

#include "ocudu/support/ocudu_assert.h"
#include "fmt/format.h"
#include <cstdint>

namespace ocudu {
namespace fapi {

/// Message type IDs.
enum class message_type_id : uint16_t {
  param_request      = 0x00,
  param_response     = 0x01,
  config_request     = 0x02,
  config_response    = 0x03,
  start_request      = 0x04,
  stop_request       = 0x05,
  stop_indication    = 0x06,
  error_indication   = 0x07,
  dl_tti_request     = 0x80,
  dl_tti_response    = 0x8a,
  ul_tti_request     = 0x81,
  slot_indication    = 0x82,
  ul_dci_request     = 0x83,
  tx_data_request    = 0x84,
  rx_data_indication = 0x85,
  crc_indication     = 0x86,
  uci_indication     = 0x87,
  srs_indication     = 0x88,
  rach_indication    = 0x89
};

/// Returns a string representation for the given message identifier.
inline const char* get_message_type_string(message_type_id msg_id)
{
  switch (msg_id) {
    case message_type_id::rach_indication:
      return "RACH.indication";
    case message_type_id::crc_indication:
      return "CRC.indication";
    case message_type_id::error_indication:
      return "ERROR.indication";
    case message_type_id::dl_tti_request:
      return "DL_TTI.request";
    case message_type_id::tx_data_request:
      return "Tx_Data.request";
    case message_type_id::ul_dci_request:
      return "UL_DCI.request";
    case message_type_id::uci_indication:
      return "UCI.indication";
    case message_type_id::rx_data_indication:
      return "Rx_Data.indication";
    case message_type_id::slot_indication:
      return "SLOT.indication";
    case message_type_id::config_request:
      return "CONFIG.request";
    case message_type_id::config_response:
      return "CONFIG.response";
    case message_type_id::dl_tti_response:
      return "DL_TTI.response";
    case message_type_id::param_request:
      return "PARAM.request";
    case message_type_id::param_response:
      return "PARAM.response";
    case message_type_id::srs_indication:
      return "SRS.indication";
    case message_type_id::start_request:
      return "START.request";
    case message_type_id::stop_indication:
      return "STOP.indication";
    case message_type_id::stop_request:
      return "STOP.request";
    case message_type_id::ul_tti_request:
      return "UL_TTI.request";
    default:
      ocudu_assert(0, "Invalid FAPI message type={}", fmt::underlying(msg_id));
      break;
  }
  return "";
}

/// Main PHY API message header.
struct message_header {
  uint8_t num_messages;
  uint8_t handle;
  uint8_t padding[2];
};

/// Common PHY API message structure.
struct base_message {
  message_type_id message_type;
  uint8_t         padding[2];
  uint32_t        length;
};

} // namespace fapi
} // namespace ocudu
