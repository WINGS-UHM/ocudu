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

#include "ocudu/pdcp/pdcp_t_reordering.h"
#include "ocudu/rrc/meas_types.h"

namespace ocudu::ocucp {

/// PDCP configuration for a SRB.
struct srb_pdcp_config {
  /// Value in ms of t-Reordering specified in TS 38.323.
  pdcp_t_reordering t_reordering = pdcp_t_reordering::infinity;
};

/// RRC UE configuration.
struct rrc_ue_cfg_t {
  /// PDCP configuration for SRB1.
  srb_pdcp_config              srb1_pdcp_cfg;
  std::vector<rrc_meas_timing> meas_timings;
  bool                         force_reestablishment_fallback = false;
  bool                         force_resume_fallback          = false;
  /// \brief Guard time used for RRC message exchange with UE.
  std::chrono::milliseconds rrc_procedure_guard_time_ms{500};
};

} // namespace ocudu::ocucp
