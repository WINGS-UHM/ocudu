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

#include "ocudu/fapi/p7/messages/dl_pdu_type.h"
#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"

namespace ocudu {
namespace fapi {

struct validator_report;

/// Validates the given transmission precoding and beamforming PDU and returns true on success, otherwise false.
bool validate_tx_precoding_and_beamforming_pdu(const tx_precoding_and_beamforming_pdu& pdu,
                                               validator_report&                       report,
                                               dl_pdu_type                             pdu_type);

} // namespace fapi
} // namespace ocudu
