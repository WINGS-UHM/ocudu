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

#include "ocudu/adt/static_vector.h"

namespace ocudu {
namespace fapi {

/// Precoding and beamforming PDU.
struct tx_precoding_and_beamforming_pdu {
  /// Maximum number of digital beamforming interfaces supported. Implementation defined.
  static constexpr unsigned MAX_NUM_SUPPORTED_DIGITAL_BEAMFORMING_INTERFACES = 4U;
  /// Maximum number of PRGs supported. Implementation defined.
  static constexpr unsigned MAX_NUM_PRGS = 1U;

  /// Physical resource groups information.
  struct prgs_info {
    uint16_t                                                                  pm_index;
    static_vector<uint16_t, MAX_NUM_SUPPORTED_DIGITAL_BEAMFORMING_INTERFACES> beam_index;
  };

  /// Setting this variable to a value other than 0 marks the struct as not being used.
  uint8_t                                trp_scheme = 1U;
  uint16_t                               prg_size;
  uint8_t                                dig_bf_interfaces;
  static_vector<prgs_info, MAX_NUM_PRGS> prgs;
};

} // namespace fapi
} // namespace ocudu
