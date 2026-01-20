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

#include "ocudu/fapi/common/base_message.h"
#include "ocudu/fapi/p7/messages/ul_pdu_type.h"
#include "ocudu/fapi/p7/messages/ul_prach_pdu.h"
#include "ocudu/fapi/p7/messages/ul_pucch_pdu.h"
#include "ocudu/fapi/p7/messages/ul_pusch_pdu.h"
#include "ocudu/fapi/p7/messages/ul_srs_pdu.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"

namespace ocudu {
namespace fapi {

/// Common uplink PDU information.
struct ul_tti_request_pdu {
  ul_pdu_type pdu_type;
  uint16_t    pdu_size;

  // :TODO: add variant for the PDUs below.
  ul_prach_pdu prach_pdu;
  ul_pusch_pdu pusch_pdu;
  ul_pucch_pdu pucch_pdu;
  ul_srs_pdu   srs_pdu;
};

/// Uplink TTI request message.
struct ul_tti_request : public base_message {
  enum class pdu_type : uint8_t { PRACH, PUSCH, PUCCH_format01, PUCCH_format234, SRS, msga_PUSCH };

  /// Maximum number of supported UL PDU types in this release.
  static constexpr unsigned MAX_NUM_UL_TYPES = 6;

  slot_point                                              slot;
  std::array<uint16_t, MAX_NUM_UL_TYPES>                  num_pdus_of_each_type;
  uint16_t                                                num_groups;
  static_vector<ul_tti_request_pdu, MAX_UL_PDUS_PER_SLOT> pdus;
  //: TODO: groups array
};

} // namespace fapi
} // namespace ocudu
