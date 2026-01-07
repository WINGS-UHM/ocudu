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

#include "ocudu/fapi/p7/builders/dl_ssb_pdu_builder.h"
#include "ocudu/mac/mac_cell_result.h"

namespace ocudu {
namespace fapi_adaptor {

/// \brief Helper function that converts from a SSB MAC PDU to a SSB FAPI PDU.
///
/// \param[out] fapi_pdu  SSB FAPI PDU that will store the converted data.
/// \param[in] mac_pdu    SSB MAC PDU to convert to SSB FAPI PDU.
/// \param[in] slot       Slot point associated to this PDU.
void convert_ssb_mac_to_fapi(fapi::dl_ssb_pdu& fapi_pdu, const ocudu::dl_ssb_pdu& mac_pdu, slot_point slot);

/// \brief Helper function that converts from a SSB MAC PDU to a SSB FAPI PDU.
///
/// \param[out] builder   SSB FAPI builder that helps to fill the PDU.
/// \param[in] mac_pdu    SSB MAC PDU to convert to SSB FAPI PDU.
/// \param[in] slot       Slot point associated to this PDU.
void convert_ssb_mac_to_fapi(fapi::dl_ssb_pdu_builder& builder, const ocudu::dl_ssb_pdu& mac_pdu, slot_point slot);

} // namespace fapi_adaptor
} // namespace ocudu
