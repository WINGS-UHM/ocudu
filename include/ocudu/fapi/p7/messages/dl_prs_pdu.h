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

#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/prs/prs.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// Downlink PRS PDU information.
struct dl_prs_pdu {
  subcarrier_spacing               scs;
  cyclic_prefix                    cp;
  uint16_t                         nid_prs;
  prs_comb_size                    comb_size;
  uint8_t                          comb_offset;
  prs_num_symbols                  num_symbols;
  uint8_t                          first_symbol;
  crb_interval                     crbs;
  std::optional<float>             prs_power_offset;
  tx_precoding_and_beamforming_pdu precoding_and_beamforming;
};

} // namespace fapi
} // namespace ocudu
