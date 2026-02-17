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

#include "ocudu/cu_up/cu_up_types.h"
#include <map>
#include <set>

namespace ocudu::ocuup {

/// Useful typedef's to get the current state of the CU-UP.
using drb_state_t         = std::map<drb_id_t, std::set<qos_flow_id_t>>;
using pdu_session_state_t = std::map<pdu_session_id_t, drb_state_t>;
using up_state_t          = std::map<cu_up_ue_index_t, pdu_session_state_t>;

} // namespace ocudu::ocuup
