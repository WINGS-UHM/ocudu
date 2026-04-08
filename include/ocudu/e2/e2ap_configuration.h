// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/gnb_cu_up_id.h"
#include "ocudu/ran/gnb_du_id.h"
#include "ocudu/ran/gnb_id.h"
#include <optional>
#include <string>

namespace ocudu {

/// \brief E2AP configuration
struct e2ap_configuration {
  gnb_id_t                      gnb_id = {0, 22};
  std::string                   plmn; /// Full PLMN as string (without possible filler digit) e.g. "00101"
  std::optional<gnb_du_id_t>    gnb_du_id;
  std::optional<gnb_cu_up_id_t> gnb_cu_up_id;
  unsigned                      max_setup_retries = 5;
  bool                          e2sm_kpm_enabled  = false;
  bool                          e2sm_rc_enabled   = false;
  bool                          e2sm_ccc_enabled  = false;
};

} // namespace ocudu
