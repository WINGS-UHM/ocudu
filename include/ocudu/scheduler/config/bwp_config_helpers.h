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

#include "pucch_resource_builder_params.h"
#include "ue_bwp_config.h"

namespace ocudu::bwp_config_helpers {

// \brief Get the position of a given Resource Set ID 0 resource in the cell PUCCH resource list.
//
// \param params The PUCCH resource builder parameters.
// \param res_set_cfg_id The resource set config index.
// \param pri The index of the resource within the resource set.
// \return The index of the PUCCH resource in the cell PUCCH resource list.
inline unsigned get_pucch_res_set_0_cell_res_idx(const pucch_resource_builder_params& params,
                                                 pucch_resource_set_config_id         res_set_cfg_id,
                                                 unsigned                             pri)
{
  ocudu_assert(res_set_cfg_id.value() < params.nof_cell_res_set_configs,
               "Resource set config index={} exceeds configured number of resource set configs={}",
               res_set_cfg_id.value(),
               params.nof_cell_res_set_configs);
  ocudu_assert(pri < params.res_set_0_size.value(),
               "Resource index={} exceeds configured resource set size={}",
               pri,
               params.res_set_0_size.value());
  return res_set_cfg_id.value() * params.res_set_0_size.value() + pri;
}

// \brief Get the position of a given Resource Set ID 1 resource in the cell PUCCH resource list.
//
// \param params The PUCCH resource builder parameters.
// \param res_set_cfg_id The resource set config index.
// \param pri The index of the resource within the resource set.
// \return The index of the PUCCH resource in the cell PUCCH resource list.
inline unsigned get_pucch_res_set_1_cell_res_idx(const pucch_resource_builder_params& params,
                                                 pucch_resource_set_config_id         res_set_cfg_id,
                                                 unsigned                             pri)
{
  ocudu_assert(res_set_cfg_id.value() < params.nof_cell_res_set_configs,
               "Resource set config index={} exceeds configured number of resource set configs={}",
               res_set_cfg_id.value(),
               params.nof_cell_res_set_configs);
  ocudu_assert(pri < params.res_set_1_size.value(),
               "Resource index={} exceeds configured resource set size={}",
               pri,
               params.res_set_1_size.value());
  return params.nof_cell_res_set_configs * params.res_set_0_size.value() + params.nof_cell_sr_resources +
         res_set_cfg_id.value() * params.res_set_1_size.value() + pri;
}

// \brief Get the position of a given PUCCH resource for SR in the cell PUCCH resource list.
//
// \param params The PUCCH resource builder parameters.
// \param sr_res_id The SR PUCCH resource index.
// \return The index of the PUCCH resource in the cell PUCCH resource list.
inline unsigned get_pucch_sr_cell_res_idx(const pucch_resource_builder_params& params, pucch_sr_resource_id sr_res_id)
{
  ocudu_assert(sr_res_id.value() < params.nof_cell_sr_resources,
               "SR resource index={} exceeds configured number of SR resources={}",
               sr_res_id.value(),
               params.nof_cell_sr_resources);
  return params.nof_cell_res_set_configs * params.res_set_0_size.value() + sr_res_id.value();
}

// \brief Get the position of a given PUCCH resource for CSI in the cell PUCCH resource list.
//
// \param params The PUCCH resource builder parameters.
// \param csi_res_id The CSI PUCCH resource index.
// \return The index of the PUCCH resource in the cell PUCCH resource list.
inline unsigned get_pucch_csi_cell_res_idx(const pucch_resource_builder_params& params,
                                           pucch_csi_resource_id                csi_res_id)
{
  ocudu_assert(csi_res_id.value() < params.nof_cell_csi_resources,
               "CSI resource index={} exceeds configured number of CSI resources={}",
               csi_res_id.value(),
               params.nof_cell_csi_resources);
  return params.nof_cell_res_set_configs * params.res_set_0_size.value() + params.nof_cell_sr_resources +
         params.nof_cell_res_set_configs * params.res_set_1_size.value() + csi_res_id.value();
}

} // namespace ocudu::bwp_config_helpers
