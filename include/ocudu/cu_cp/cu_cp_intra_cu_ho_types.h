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

#include "cu_cp_types.h"

namespace ocudu::ocucp {

struct cu_cp_intra_cu_handover_request {
  ue_index_t          source_ue_index = ue_index_t::invalid;
  du_index_t          target_du_index = du_index_t::invalid;
  nr_cell_global_id_t cgi;
  pci_t               target_pci = INVALID_PCI;
};

struct cu_cp_intra_cu_handover_response {
  bool success = false;
};

} // namespace ocudu::ocucp
