// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/f1ap/f1ap_ue_id_types.h"

namespace ocudu {
namespace ocucp {

/// Identifiers that associate a UE in the F1AP-CU.
struct f1ap_ue_ids {
  ue_index_t                         ue_index      = ue_index_t::invalid;
  gnb_cu_ue_f1ap_id_t                cu_ue_f1ap_id = gnb_cu_ue_f1ap_id_t::invalid;
  std::optional<gnb_du_ue_f1ap_id_t> du_ue_f1ap_id;
};

} // namespace ocucp
} // namespace ocudu
