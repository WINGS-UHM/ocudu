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

#include "ocudu/adt/byte_buffer.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/cu_cp/up_context.h"
#include "ocudu/rrc/meas_types.h"
#include "ocudu/security/security.h"
#include <optional>

namespace ocudu {
namespace ocucp {

/// \brief RRC context transfered from one UE object to the other during mobility.
struct rrc_ue_transfer_context {
  security::security_context            sec_context;
  std::optional<rrc_meas_cfg>           meas_cfg;
  up_context                            up_ctx;
  static_vector<srb_id_t, MAX_NOF_SRBS> srbs; // List of active SRBs (TODO: add PDCP config).
  byte_buffer                           handover_preparation_info;
  byte_buffer                           ue_cap_rat_container_list;
  bool                                  is_inter_cu_handover = false;
};

} // namespace ocucp
} // namespace ocudu
