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

#include "ocudu/scheduler/config/pucch_resource_builder_params.h"
#include <vector>

namespace ocudu {

struct pucch_resource_builder_params;
struct bwp_downlink_dedicated;
struct serving_cell_config;
struct dl_config_common;
struct pdcch_config;

namespace config_helpers {

/// Builds the list of PUCCH guardbands.
std::vector<pucch_resource> build_pucch_resource_list(const pucch_resource_builder_params& user_params,
                                                      unsigned                             bwp_size);

unsigned compute_tot_nof_monitored_pdcch_candidates_per_slot(const serving_cell_config& ue_cell_cfg,
                                                             const dl_config_common&    dl_cfg_common);
unsigned compute_tot_nof_monitored_pdcch_candidates_per_slot(const pdcch_config&     ue_pdcch_cfg,
                                                             const dl_config_common& dl_cfg_common);
} // namespace config_helpers
} // namespace ocudu
