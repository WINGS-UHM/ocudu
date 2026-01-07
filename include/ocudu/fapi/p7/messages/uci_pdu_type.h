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

/// UCI PDU type ID.
enum class uci_pdu_type : uint16_t { PUSCH, PUCCH_format_0_1, PUCCH_format_2_3_4 };

/// Returns a string identifier for the given UCI.indication PDU.
inline const char* get_uci_pdu_type_string(uci_pdu_type pdu_id)
{
  switch (pdu_id) {
    case uci_pdu_type::PUSCH:
      return "PUSCH";
    case uci_pdu_type::PUCCH_format_0_1:
      return "PUCCH Format 0/1";
    case uci_pdu_type::PUCCH_format_2_3_4:
      return "PUCCH Format 2/3/4";
    default:
      ocudu_assert(0, "Invalid UCI.indication PDU={}", fmt::underlying(pdu_id));
      break;
  }
  return "";
}

} // namespace fapi
} // namespace ocudu
