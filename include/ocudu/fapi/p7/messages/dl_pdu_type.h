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

/// Downlink PDU type ID.
enum class dl_pdu_type : uint16_t { PDCCH, PDSCH, CSI_RS, SSB, PRS = 5 };

inline unsigned to_value(dl_pdu_type value)
{
  return static_cast<unsigned>(value);
}

/// Returns a string identifier for the given DL_TTI.request PDU.
inline const char* get_dl_tti_pdu_type_string(dl_pdu_type pdu_id)
{
  switch (pdu_id) {
    case dl_pdu_type::CSI_RS:
      return "CSI-RS";
    case dl_pdu_type::PDCCH:
      return "PDCCH";
    case dl_pdu_type::PDSCH:
      return "PDSCH";
    case dl_pdu_type::SSB:
      return "SSB";
    default:
      ocudu_assert(0, "Invalid DL_TTI.request PDU={}", fmt::underlying(pdu_id));
      break;
  }
  return "";
}

} // namespace fapi
} // namespace ocudu
