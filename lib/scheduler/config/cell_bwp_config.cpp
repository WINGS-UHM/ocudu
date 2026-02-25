/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/scheduler/config/cell_bwp_config.h"
#include "ocudu/scheduler/config/bwp_builder_params.h"
#include "ocudu/scheduler/config/pucch_resource_builder_params.h"
#include "ocudu/scheduler/config/pucch_resource_generator.h"
#include "ocudu/scheduler/config/ran_cell_config.h"

using namespace ocudu;

cell_bwp_config ocudu::make_cell_bwp_config(const ran_cell_config& cell_cfg)
{
  const pucch_resource_builder_params& res_params = cell_cfg.init_bwp_builder.pucch.resources;
  return cell_bwp_config{.ul = {.pucch = {.resources = config_helpers::generate_cell_pucch_res_list(
                                              res_params.res_set_0_size.value() * res_params.nof_cell_res_set_configs +
                                                  res_params.nof_cell_sr_resources,
                                              res_params.res_set_1_size.value() * res_params.nof_cell_res_set_configs +
                                                  res_params.nof_cell_csi_resources,
                                              res_params.f0_or_f1_params,
                                              res_params.f2_or_f3_or_f4_params,
                                              cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length(),
                                              res_params.max_nof_symbols)}}};
}
