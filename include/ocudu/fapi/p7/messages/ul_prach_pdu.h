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

#include "ocudu/adt/interval.h"
#include "ocudu/ran/prach/prach_format_type.h"

namespace ocudu {
namespace fapi {

/// Uplink PRACH PDU information.
struct ul_prach_pdu {
  using preambles_interval = interval<uint8_t, false>;

  uint8_t            num_prach_ocas;
  prach_format_type  prach_format;
  uint8_t            index_fd_ra;
  uint8_t            prach_start_symbol;
  uint16_t           num_cs;
  uint32_t           handle = 0U;
  uint8_t            num_fd_ra;
  preambles_interval preambles;
};

} // namespace fapi
} // namespace ocudu
