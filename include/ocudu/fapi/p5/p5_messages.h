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

#include "ocudu/fapi/cell_config.h"
#include "ocudu/fapi/common/base_message.h"
#include "ocudu/fapi/common/error_code.h"

namespace ocudu {
namespace fapi {

/// Param request message.
struct param_request : public base_message {
  uint8_t protocol_version;
};

/// Param response message.
struct param_response : public base_message {
  error_code_id error_code;
  uint8_t       num_tlv;
};

/// Config request message.
struct config_request : public base_message {
  cell_configuration cell_cfg;
};

/// Config response message.
struct config_response : public base_message {
  /// Maximum number of invalid TLVs supported.
  static constexpr unsigned MAX_NUM_TLVS = 2048;

  error_code_id error_code;
};

/// Start request message.
struct start_request : public base_message {};

/// Start response message.
struct start_response : public base_message {};

/// Stop request message.
struct stop_request : public base_message {};

/// Stop indication message.
struct stop_indication : public base_message {};

} // namespace fapi
} // namespace ocudu
