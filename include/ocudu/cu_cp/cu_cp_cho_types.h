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

#include "ocudu/adt/bounded_integer.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include <chrono>

namespace ocudu::ocucp {

/// Conditional Reconfiguration ID used by CHO procedures. Range is 1..8.
using cond_recfg_id_t = bounded_integer<uint8_t, 1, 8>;

/// \brief Single target candidate for intra-CU CHO orchestration.
struct cu_cp_cho_target_candidate {
  pci_t               pci = INVALID_PCI;
  nr_cell_global_id_t cgi;
  du_index_t          du_index = du_index_t::invalid;
};

/// \brief Request for intra-CU CHO orchestration.
struct cu_cp_intra_cu_cho_request {
  ue_index_t                              source_ue_index = ue_index_t::invalid;
  du_index_t                              source_du_index = du_index_t::invalid;
  std::vector<cu_cp_cho_target_candidate> targets;
  std::chrono::milliseconds               timeout = std::chrono::milliseconds{10000};
};

/// \brief Response from intra-CU CHO orchestration.
struct cu_cp_intra_cu_cho_response {
  bool success = false;
};

} // namespace ocudu::ocucp
