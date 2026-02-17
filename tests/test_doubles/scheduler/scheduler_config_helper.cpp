/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "scheduler_config_helper.h"
#include "ocudu/scheduler/config/logical_channel_config_factory.h"
#include "ocudu/scheduler/config/ran_cell_config_helper.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"

using namespace ocudu;

sched_cell_configuration_request_message
sched_config_helper::make_default_sched_cell_configuration_request(const cell_config_builder_params& params_input)
{
  config_helpers::cell_config_builder_params_extended params{params_input};
  sched_cell_configuration_request_message            sched_req{};
  sched_req.cell_index = to_du_cell_index(0);
  sched_req.ran        = config_helpers::make_default_ran_cell_config(params);

  // SIB1 parameters.
  sched_req.si_scheduling.sib1_payload_size = units::bytes{101}; // Random size.

  if (params.csi_rs_enabled) {
    csi_helper::csi_meas_config_builder_params csi_params = config_helpers::make_default_csi_builder_params(params);
    sched_req.ran.init_bwp_builder.csi                    = csi_params.csi_params;
  }

  return sched_req;
}

cell_config_dedicated
sched_config_helper::create_test_initial_ue_spcell_cell_config(const cell_config_builder_params& params)
{
  cell_config_dedicated cfg;
  cfg.serv_cell_idx = to_serv_cell_index(0);
  cfg.serv_cell_cfg = config_helpers::create_default_initial_ue_serving_cell_config(params);
  return cfg;
}

sched_ue_creation_request_message
sched_config_helper::create_default_sched_ue_creation_request(const cell_config_builder_params& params,
                                                              span<const lcid_t>                lcid_to_cfg)
{
  sched_ue_creation_request_message msg{};

  msg.ue_index = to_du_ue_index(0);
  msg.crnti    = to_rnti(0x4601);

  scheduling_request_to_addmod sr_0{.sr_id = scheduling_request_id::SR_ID_MIN, .max_tx = sr_max_tx::n64};
  msg.cfg.sched_request_config_list.emplace();
  msg.cfg.sched_request_config_list->push_back(sr_0);

  msg.cfg.cells.emplace();
  msg.cfg.cells->push_back(create_test_initial_ue_spcell_cell_config(params));

  msg.cfg.lc_config_list.emplace();
  msg.cfg.lc_config_list->resize(2);
  (*msg.cfg.lc_config_list)[0] = config_helpers::create_default_logical_channel_config(lcid_t::LCID_SRB0);
  (*msg.cfg.lc_config_list)[1] = config_helpers::create_default_logical_channel_config(lcid_t::LCID_SRB1);
  for (lcid_t lcid : lcid_to_cfg) {
    if (lcid >= lcid_t::LCID_SRB2) {
      msg.cfg.lc_config_list->push_back(config_helpers::create_default_logical_channel_config(lcid));
    }
  }

  return msg;
}

sched_ue_creation_request_message
sched_config_helper::create_default_sched_ue_creation_request(const cell_config_builder_params&    params,
                                                              const std::initializer_list<lcid_t>& lcid_to_cfg)
{
  return create_default_sched_ue_creation_request(params, span<const lcid_t>(lcid_to_cfg.begin(), lcid_to_cfg.end()));
}

sched_ue_creation_request_message
sched_config_helper::create_empty_spcell_cfg_sched_ue_creation_request(const cell_config_builder_params& params)
{
  sched_ue_creation_request_message msg{};

  msg.ue_index = to_du_ue_index(0);
  msg.crnti    = to_rnti(0x4601);

  cell_config_dedicated cfg;
  cfg.serv_cell_idx              = to_serv_cell_index(0);
  serving_cell_config& serv_cell = cfg.serv_cell_cfg;

  serv_cell.cell_index = to_du_cell_index(0);

  msg.cfg.cells.emplace();
  msg.cfg.cells->push_back(cfg);

  msg.cfg.lc_config_list.emplace();
  msg.cfg.lc_config_list->resize(1);
  (*msg.cfg.lc_config_list)[0] = config_helpers::create_default_logical_channel_config(lcid_t::LCID_SRB0);

  return msg;
}
