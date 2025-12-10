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

#include "ocudu/fapi/p7/messages/ul_srs_pdu.h"
#include "ocudu/phy/upper/uplink_pdu_slot_repository.h"

namespace ocudu {
namespace fapi_adaptor {

/// Helper function that converts an SRS FAPI PDU into an SRS uplink slot PDU using the system frame number and slot.
void convert_srs_fapi_to_phy(uplink_pdu_slot_repository::srs_pdu& pdu,
                             const fapi::ul_srs_pdu&              fapi_pdu,
                             unsigned                             sector_id_,
                             unsigned                             nof_rx_antennas,
                             slot_point                           slot);

} // namespace fapi_adaptor
} // namespace ocudu
