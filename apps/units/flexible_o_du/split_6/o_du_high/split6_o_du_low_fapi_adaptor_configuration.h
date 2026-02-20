// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/fapi/cell_config.h"
#include "ocudu/ran/pdsch/pdsch_time_domain_resource.h"
#include "ocudu/ran/pusch/pusch_time_domain_resource.h"

namespace ocudu {
namespace fapi_adaptor {

/// Split 6 FAPI-specific cell configuration.
struct split6_o_du_low_fapi_adaptor_cell_config {
  subcarrier_spacing                                 scs_common;
  uint16_t                                           num_tx_ant;
  dmrs_typeA_position                                dmrs_typeA_pos;
  bool                                               enable_csi_rs_pdsch_multiplexing;
  std::vector<pdsch_time_domain_resource_allocation> pdsch_td_alloc_list;
  std::vector<pusch_time_domain_resource_allocation> pusch_td_alloc_list;
};

/// Split 6 FAPI-specific adaptor configuration.
struct split6_o_du_low_fapi_adaptor_configuration {
  std::vector<split6_o_du_low_fapi_adaptor_cell_config> cells;
};

} // namespace fapi_adaptor
} // namespace ocudu
