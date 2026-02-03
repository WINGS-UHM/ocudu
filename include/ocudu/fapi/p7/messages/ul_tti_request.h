// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

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
  std::variant<ul_prach_pdu, ul_pusch_pdu, ul_pucch_pdu, ul_srs_pdu> pdu;
};

/// Uplink TTI request message.
struct ul_tti_request {
  slot_point                                              slot;
  static_vector<ul_tti_request_pdu, MAX_UL_PDUS_PER_SLOT> pdus;
};

} // namespace fapi
} // namespace ocudu
