
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
#include "ocudu/fapi/p7/messages/dl_csi_rs_pdu.h"
#include "ocudu/fapi/p7/messages/dl_pdcch_pdu.h"
#include "ocudu/fapi/p7/messages/dl_pdsch_pdu.h"
#include "ocudu/fapi/p7/messages/dl_pdu_type.h"
#include "ocudu/fapi/p7/messages/dl_prs_pdu.h"
#include "ocudu/fapi/p7/messages/dl_ssb_pdu.h"

namespace ocudu {
namespace fapi {

/// Common downlink PDU information.
struct dl_tti_request_pdu {
  dl_pdu_type pdu_type;

  // :TODO: add these fields in a std::variant.
  dl_pdcch_pdu  pdcch_pdu;
  dl_pdsch_pdu  pdsch_pdu;
  dl_csi_rs_pdu csi_rs_pdu;
  dl_ssb_pdu    ssb_pdu;
  dl_prs_pdu    prs_pdu;
};

/// Downlink TTI request message.
struct dl_tti_request : public base_message {
  /// Array index for the number of DL DCIs.
  static constexpr unsigned DL_DCI_INDEX = 4;
  /// Maximum supported number of DL PDU types in this release.
  static constexpr unsigned                               MAX_NUM_DL_TYPES = 6;
  slot_point                                              slot;
  std::array<uint16_t, MAX_NUM_DL_TYPES>                  num_pdus_of_each_type = {};
  static_vector<dl_tti_request_pdu, MAX_DL_PDUS_PER_SLOT> pdus;
  //: TODO: groups array
  //: TODO: top level rate match patterns
};

} // namespace fapi
} // namespace ocudu
