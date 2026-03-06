// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/p7/messages/dl_csi_rs_pdu.h"
#include "ocudu/fapi/p7/messages/dl_pdcch_pdu.h"
#include "ocudu/fapi/p7/messages/dl_pdsch_pdu.h"
#include "ocudu/fapi/p7/messages/dl_prs_pdu.h"
#include "ocudu/fapi/p7/messages/dl_ssb_pdu.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"

namespace ocudu {
namespace fapi {

/// Common downlink PDU information.
struct dl_tti_request_pdu {
  std::variant<dl_pdcch_pdu, dl_pdsch_pdu, dl_csi_rs_pdu, dl_ssb_pdu, dl_prs_pdu> pdu;
};

/// Downlink TTI request message.
struct dl_tti_request {
  slot_point                                              slot;
  static_vector<dl_tti_request_pdu, MAX_DL_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
