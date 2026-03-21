// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "ocudu/scheduler/config/cell_bwp_config.h"
#include "ocudu/scheduler/config/bwp_builder_params.h"
#include "ocudu/scheduler/config/pucch_resource_generator.h"
#include "ocudu/scheduler/config/ran_cell_config.h"

using namespace ocudu;

cell_bwp_config ocudu::make_cell_bwp_config(const ran_cell_config& cell_cfg)
{
  return cell_bwp_config{.ul = {.pucch = {.resources = config_helpers::generate_cell_pucch_res_list(
                                              cell_cfg.init_bwp.pucch.resources,
                                              cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length())}}};
}
