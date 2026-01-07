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

#include "cell_configuration.h"
#include "logical_channel_config_pool.h"
#include "sched_config_params.h"
#include "ocudu/ran/du_types.h"
#include "ocudu/scheduler/config/bwp_configuration.h"
#include "ocudu/scheduler/config/serving_cell_config.h"
#include <map>

namespace ocudu {

struct sched_cell_configuration_request_message;
struct sched_ue_creation_request_message;
struct sched_ue_reconfiguration_message;
struct cell_config_dedicated;

class du_cell_config_pool
{
public:
  du_cell_config_pool(const scheduler_expert_config& sched_cfg_, const sched_cell_configuration_request_message& req);
  du_cell_config_pool(const du_cell_config_pool&)            = delete;
  du_cell_config_pool& operator=(const du_cell_config_pool&) = delete;

  const cell_configuration& cell_cfg() const { return cell_cfg_inst; }
  cell_configuration&       cell_cfg() { return cell_cfg_inst; }

  ue_cell_config_ptr update_ue(const serving_cell_config& ue_cell);

private:
  void add_bwp(ue_cell_res_config&           out,
               bwp_id_t                      bwp_id,
               const bwp_downlink_common&    dl_bwp_common,
               const bwp_downlink_dedicated& dl_bwp_ded,
               const bwp_uplink_common*      ul_bwp_common,
               const bwp_uplink_dedicated*   ul_bwp_ded);

  /// Cell common configuration.
  cell_configuration cell_cfg_inst;

  // Cell common BWP configurations.
  const bwp_downlink_common& init_dl_bwp;
  const bwp_uplink_common&   init_ul_bwp;

  // Pools of UE-dedicated configurations.
  config_object_pool<ue_cell_res_config>         cell_cfg_pool;
  config_object_pool<bwp_config>                 bwp_config_pool;
  config_object_pool<bwp_downlink_common>        bwp_dl_common_config_pool;
  config_object_pool<bwp_downlink_dedicated>     bwp_dl_ded_config_pool;
  config_object_pool<bwp_uplink_common>          bwp_ul_common_config_pool;
  config_object_pool<coreset_configuration>      coreset_config_pool;
  config_object_pool<search_space_configuration> ss_config_pool;
  config_object_pool<pdsch_serving_cell_config>  pdsch_serv_cell_pool;
  config_object_pool<pusch_serving_cell_config>  pusch_serv_cell_pool;
  config_object_pool<csi_meas_config>            csi_meas_config_pool;
};

/// Class responsible for managing the configurations of all the entities (cells, UEs, slices) in a cell group.
class du_cell_group_config_pool
{
public:
  /// Creates handles to the resources associated with common cell configuration.
  cell_configuration& add_cell(const scheduler_expert_config&                  expert_cfg,
                               const sched_cell_configuration_request_message& cell_cfg);

  /// Remove cell and respective resources.
  void rem_cell(du_cell_index_t cell_index);

  /// Creates handles to the resources associated with a specific UE during its creation.
  ue_creation_params add_ue(const sched_ue_creation_request_message& creation_req);

  /// Creates handles to the resources associated with a specific UE during its reconfiguration.
  ue_reconfig_params reconf_ue(const sched_ue_reconfiguration_message& cfg_req);

private:
  logical_channel_config_pool lc_ch_pool;

  slotted_id_vector<du_cell_index_t, std::unique_ptr<du_cell_config_pool>> cells;
};

} // namespace ocudu
