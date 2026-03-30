// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "logical_channel_list_config.h"
#include "sched_coreset_config.h"
#include "ocudu/adt/slotted_vector.h"
#include "ocudu/ran/du_types.h"
#include "ocudu/scheduler/config/serving_cell_config.h"
#include "ocudu/scheduler/config/ue_bwp_config.h"

namespace ocudu {

struct sched_ue_config_request;

using serving_cell_config_ptr = config_ptr<serving_cell_config>;

/// Configuration of a BWP. It aggregates both the common and dedicated configurations for DL and UL.
struct bwp_config {
  /// UE-specific BWP Identifier
  bwp_id_t bwp_id;
  /// BWP Downlink Common Configuration
  const bwp_downlink_common* dl_common = nullptr;
  /// BWP Downlink Dedicated Configuration
  std::optional<config_ptr<bwp_downlink_dedicated>> dl_ded;
  /// BWP Uplink Common Configuration
  std::optional<config_ptr<bwp_uplink_common>> ul_common;
  /// BWP Uplink Dedicated Configuration
  std::optional<bwp_uplink_dedicated> ul_ded;
  /// CoreSets associated with this BWP.
  slotted_id_vector<coreset_id, const coreset_configuration*> coresets;
  /// Search Spaces associated with this BWP.
  slotted_id_vector<search_space_id, const search_space_configuration*> search_spaces;

  ue_bwp_config bwp;

  bool operator==(const bwp_config& other) const
  {
    return bwp_id == other.bwp_id and dl_common == other.dl_common and dl_ded == other.dl_ded and
           ul_common == other.ul_common and ul_ded == other.ul_ded and coresets == other.coresets and
           search_spaces == other.search_spaces;
  }
};

using bwp_config_ptr  = config_ptr<bwp_config>;
using bwp_config_list = slotted_id_vector<bwp_id_t, bwp_config_ptr>;

/// UE-dedicated resources for a given cell.
struct ue_cell_res_config {
  /// DU-specific cell identifier.
  du_cell_index_t cell_index;
  /// Container that maps Coreset-Ids to CORESET configurations for this BWP.
  /// Note: The ID space of CoresetIds is common among the BWPs of a Serving Cell as per TS 38.331.
  slotted_id_vector<coreset_id, const coreset_configuration*> coresets;
  /// Container that maps searchSpaceIds to searchSpace configurations for this BWP.
  /// Note: The ID space of searchSpaceIds is common among the BWPs of a Serving Cell as per TS 38.331.
  slotted_id_vector<search_space_id, const search_space_configuration*> search_spaces;
  /// List of BWPs configured in this cell.
  bwp_config_list bwps;
  /// \brief \c pdsch-ServingCellConfig, used to configure UE specific PDSCH parameters that are common across the UE's
  /// BWPs of one serving cell.
  std::optional<config_ptr<pdsch_serving_cell_config>> pdsch_serv_cell_cfg;
  /// \brief \c pusch-ServingCellConfig, used to configure UE specific PUSCH parameters that are common across the UE's
  /// BWPs of one serving cell.
  std::optional<config_ptr<pusch_serving_cell_config>> pusch_serv_cell_cfg;
  /// \c CSI-MeasConfig.
  std::optional<config_ptr<csi_meas_config>> csi_meas_cfg;
  /// Timing Advance Group ID to which this cell belongs to.
  time_alignment_group::id_t tag_id{0};

  bool operator==(const ue_cell_res_config& other) const
  {
    return cell_index == other.cell_index and coresets == other.coresets and search_spaces == other.search_spaces and
           bwps == other.bwps and pdsch_serv_cell_cfg == other.pdsch_serv_cell_cfg and
           pusch_serv_cell_cfg == other.pusch_serv_cell_cfg and csi_meas_cfg == other.csi_meas_cfg and
           tag_id == other.tag_id;
  }
};
using ue_cell_config_ptr = config_ptr<ue_cell_res_config>;

/// Parameters used to generate a UE configuration object.
struct ue_creation_params {
  const sched_ue_config_request&                         cfg_req;
  logical_channel_config_list_ptr                        lc_ch_list;
  slotted_id_vector<du_cell_index_t, ue_cell_config_ptr> cells;
};

/// Parameters used to reconfigure a UE configuration object.
struct ue_reconfig_params {
  const sched_ue_config_request&                         cfg_req;
  std::optional<logical_channel_config_list_ptr>         lc_ch_list;
  slotted_id_vector<du_cell_index_t, ue_cell_config_ptr> cells;
};

} // namespace ocudu
