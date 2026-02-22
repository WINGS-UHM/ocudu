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
#include "ocudu/adt/byte_buffer.h"
#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/e1ap/common/e1ap_types.h"
#include <chrono>
#include <optional>

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

/// \brief Parameters for a single CHO candidate preparation request.
struct cu_cp_cho_preparation_request {
  cond_recfg_id_t cond_recfg_id = 1; ///< CHO conditional reconfiguration ID (valid range 1..8).
};

/// \brief Result of a single CHO candidate preparation.
/// Only populated when cu_cp_intra_cu_handover_request::cho_preparation is set.
struct cu_cp_cho_preparation_result {
  ue_index_t  target_ue_index = ue_index_t::invalid; ///< Target UE allocated/prepared for this candidate.
  byte_buffer packed_rrc_recfg;                      ///< Packed RRCReconfiguration for deferred CHO execution.
  unsigned    transaction_id = 0;                    ///< RRC transaction ID of packed_rrc_recfg.
  /// CU-UP bearer update payload collected during target preparation.
  std::optional<e1ap_ng_ran_bearer_context_mod_request> ng_ran_bearer_context_mod_request;
};

} // namespace ocudu::ocucp
