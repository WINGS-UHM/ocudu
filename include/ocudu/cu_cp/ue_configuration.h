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

#include <chrono>

namespace ocudu::ocucp {

/// UE configuration passed to CU-CP
struct ue_configuration {
  std::chrono::seconds inactivity_timer{7200};
  /// Timeout for requesting a PDU session in seconds, before the UE is released.
  std::chrono::seconds request_pdu_session_timeout = std::chrono::seconds{2};
  /// Number of bits used for UE ID in I-RNTI.
  uint8_t nof_i_rnti_ue_bits = 13;
};

} // namespace ocudu::ocucp
