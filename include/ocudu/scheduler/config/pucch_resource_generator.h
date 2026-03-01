// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/adt/expected.h"
#include "ocudu/scheduler/config/pucch_resource_builder_params.h"
#include "ocudu/scheduler/config/ue_bwp_config.h"

namespace ocudu {

struct serving_cell_config;

namespace config_helpers {

/// \brief Validates the user-defined parameters for building the cell PUCCH resource list.
/// \param[in] params PUCCH resource builder parameters.
/// \param[in] bwp_size_rbs size of the BWP in RBs.
/// \return An error message if the parameters are not valid. Otherwise, success.
error_type<std::string> pucch_parameters_validator(const pucch_resource_builder_params& params, unsigned bwp_size_rbs);

/// \brief Generates the list of cell PUCCH resources from the PUCCH resource builder parameters.
///
/// The generated resources are packed on both ends of the BWP as tightly as possible (using multiplexing if
/// configured), while ensuring they don't collide with each other.
/// The resources for each UCI type are indexed according to the rules defined in \c pucch_resource_builder_params.
///
/// \param[in] params PUCCH resource builder parameters.
/// \param[in] bwp_size_rbs size of the BWP in RBs.
/// \return The list of PUCCH resources for a cell.
/// \remark The function returns an empty list in the following cases:
///         (i) If overall the RBs occupancy is larger than the BWP size.
///         (ii) If F2 intra-slot frequency hopping is enabled with only 1 symbol.
std::vector<pucch_resource> generate_cell_pucch_res_list(const pucch_resource_builder_params& params,
                                                         unsigned                             bwp_size_rbs);

/// \brief Builds the PUCCH configuration for a given UE.
///
/// This function generates the list of PUCCH resources for a given UE, including the resources for HARQ-ACK, SR and
/// CSI. It also updates the PUCCH resource sets accordingly, as well as the pointers to the PUCCH F0/F1 resource for SR
/// and to the PUCCH F2/F3/F4 resource for CSI. This function overwrites the default \c ServingCellConfig passed as a
/// function input.
///
/// The UE's PUCCH resource list composed of:
/// - \ref nof_ue_pucch_f0_f1_res_harq PUCCH Format 0/1 resources for HARQ-ACK reporting, chosen from
///   \ref nof_harq_pucch_sets possible sets of PUCCH Format 0/1 cell resources.
/// - 1 PUCCH Format 0/1 resource for SR chosen from \ref nof_cell_pucch_f0_f1_res_sr possible sets of PUCCH Format 0/1
///   cell resources.
/// - \ref nof_ue_pucch_f2_f3_f4_res_harq PUCCH Format 2/3/4 resources for HARQ-ACK reporting, chosen from
///   \ref nof_harq_pucch_sets possible sets of PUCCH Format 2/3/4 cell resources.
/// - 1 PUCCH Format 2/3/4 resource for CSI chosen from \ref nof_cell_pucch_f2_f3_f4_res_csi possible sets of PUCCH
///   Format 2/3/4 cell resources.
///
/// The returned UE PUCCH resource list \ref pucch_res_list contains the following resources, sorted as follows:
///       [ F0/F1-HARQ_0 ... F0/F1-HARQ_N-1 F0/F1-SR F2/F3/F4-HARQ_0 ... F2/F3/F4-HARQ_M-1 F2/F3/F4-CSI ]
/// where N = nof_ue_pucch_f0_f1_res_harq and M = nof_ue_pucch_f2_f3_f4_res_harq,
/// and with the following indices \ref res_id:
/// - The first \ref nof_ue_pucch_f0_f1_res_harq are the PUCCH F0/F1 resources for HARQ-ACK and have index
///   [ (cell_harq_set_idx % nof_harq_pucch_sets) * nof_ue_pucch_f0_f1_res_harq,
///     (cell_harq_set_idx % nof_harq_pucch_sets) * nof_ue_pucch_f0_f1_res_harq + nof_ue_pucch_f0_f1_res_harq ).
/// - The next resource in the list is the PUCCH F0/F1 resource for SR, which have index:
///      nof_harq_pucch_sets * nof_ue_pucch_f0_f1_res_harq + cell_sr_res_idx % nof_cell_pucch_f0_f1_res_sr.
/// - The next \ref nof_ue_pucch_f2_f3_f4_res_harq are the PUCCH F2/F3/F4 resources for HARQ-ACK and have index
///   [  nof_harq_pucch_sets * nof_ue_pucch_f0_f1_res_harq + nof_cell_pucch_f0_f1_res_sr +
///                     (cell_harq_set_idx % nof_harq_pucch_sets) * nof_ue_pucch_f2_f3_f4_res_harq,
///      nof_harq_pucch_sets * nof_ue_pucch_f0_f1_res_harq + nof_cell_pucch_f0_f1_res_sr +
///                     (cell_harq_set_idx % nof_harq_pucch_sets) * nof_ue_pucch_f2_f3_f4_res_harq +
///                     nof_ue_pucch_f2_f3_f4_res_harq).
/// - The last resource in the list is the PUCCH F2/F3/F4 resource for CSI, which has index:
////     nof_harq_pucch_sets * nof_ue_pucch_f0_f1_res_harq + nof_cell_pucch_f0_f1_res_sr +
///                     nof_ue_pucch_f2_f3_f4_res_harq * nof_harq_pucch_sets + cell_csi_res_idx %
///                     nof_cell_pucch_f2_f3_f4_res_csi.
///
/// \param[in,out] serv_cell_cfg default \c ServingCellConfig that will be overwritten by this function.
/// \param[in] cell_res_list cell PUCCH resource list from which the function picks the UE PUCCH resources.
/// \param[in] params PUCCH resource builder parameters.
/// \param[in] pucch_cfg UE PUCCH configuration parameters.
/// \param[in] periodic_csi_cfg UE periodic CSI report configuration parameters.
/// \param[in] res_set_cfg_id ID of the resource set configuration to assign to this UE.
/// \param[in] sr_res_id ID of the SR resource to assign to this UE.
/// \param[in] csi_res_id ID of the CSI resource to assign to this UE, if periodic CSI report is configured.
/// \return true if the building is successful, false otherwise.
bool ue_pucch_config_builder(serving_cell_config&                         serv_cell_cfg,
                             const std::vector<pucch_resource>&           cell_res_list,
                             const pucch_resource_builder_params&         params,
                             const ue_pucch_config&                       pucch_cfg,
                             const std::optional<ue_periodic_csi_config>& csi_cfg);

} // namespace config_helpers
} // namespace ocudu
