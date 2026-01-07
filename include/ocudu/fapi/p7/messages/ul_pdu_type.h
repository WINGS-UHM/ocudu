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

/// Uplink PDU type ID.
enum class ul_pdu_type : uint16_t { PRACH, PUSCH, PUCCH, SRS };

/// Returns a string identifier for the given UL_TTI.request PDU.
inline const char* get_ul_tti_pdu_type_string(ul_pdu_type pdu_id)
{
  switch (pdu_id) {
    case ul_pdu_type::PUSCH:
      return "PUSCH";
    case ul_pdu_type::PRACH:
      return "PRACH";
    case ul_pdu_type::PUCCH:
      return "PUCCH";
    case ul_pdu_type::SRS:
      return "SRS";
    default:
      ocudu_assert(0, "Invalid UL_TTI.request PDU={}", fmt::underlying(pdu_id));
      break;
  }
  return "";
}

} // namespace fapi
} // namespace ocudu
