/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cell_configuration.h"
#include "ocudu/ran/band_helper.h"
#include "ocudu/ran/resource_block.h"
#include "ocudu/ran/ssb/ssb_mapping.h"
#include "ocudu/scheduler/config/csi_helper.h"
#include "ocudu/scheduler/config/time_domain_resource_helper.h"
#include <algorithm>

using namespace ocudu;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::vector<zp_csi_rs_resource> make_zp_csi_rs_list(const sched_cell_configuration_request_message& msg)
{
  if (!msg.ran.init_bwp_builder.csi.has_value()) {
    return {};
  }

  csi_helper::csi_meas_config_builder_params csi_params{};
  csi_params.pci            = msg.ran.pci;
  csi_params.nof_rbs        = msg.ran.ul_cfg_common.init_ul_bwp.generic_params.crbs.length();
  csi_params.nof_ports      = msg.ran.dl_carrier.nof_ant;
  csi_params.max_nof_layers = msg.ran.init_bwp_builder.pdsch.max_nof_layers.value_or(csi_params.nof_ports);
  csi_params.mcs_table      = msg.ran.init_bwp_builder.pdsch.mcs_table;
  csi_params.csi_params     = msg.ran.init_bwp_builder.csi.value();

  return csi_helper::make_periodic_zp_csi_rs_resource_list(csi_params);
}

cell_configuration::cell_configuration(const scheduler_expert_config&                  expert_cfg_,
                                       const sched_cell_configuration_request_message& msg) :
  expert_cfg(expert_cfg_),
  cell_index(msg.cell_index),
  cell_group_index(msg.cell_group_index),
  pci(msg.ran.pci),
  scs_common(msg.ran.dl_cfg_common.init_dl_bwp.generic_params.scs),
  nof_dl_prbs(
      get_max_Nprb(msg.ran.dl_carrier.carrier_bw, scs_common, band_helper::get_freq_range(msg.ran.dl_carrier.band))),
  nof_ul_prbs(
      get_max_Nprb(msg.ran.ul_carrier.carrier_bw, scs_common, band_helper::get_freq_range(msg.ran.dl_carrier.band))),
  nof_slots_per_frame(get_nof_slots_per_subframe(msg.ran.dl_cfg_common.init_dl_bwp.generic_params.scs) *
                      NOF_SUBFRAMES_PER_FRAME),
  dl_cfg_common(msg.ran.dl_cfg_common),
  ul_cfg_common(msg.ran.ul_cfg_common),
  tdd_cfg_common(msg.ran.tdd_ul_dl_cfg_common),
  dl_carrier(msg.ran.dl_carrier),
  ssb_cfg(msg.ran.ssb_cfg),
  dmrs_typeA_pos(msg.ran.dmrs_typeA_pos),
  ul_carrier(msg.ran.ul_carrier),
  init_bwp_res(pci, to_bwp_id(0), dl_cfg_common.init_dl_bwp, nullptr),
  ded_pucch_resources(msg.ded_pucch_resources),
  zp_csi_rs_list(make_zp_csi_rs_list(msg)),
  nzp_csi_rs_list(msg.nzp_csi_rs_res_list),
  dl_data_to_ul_ack(time_domain_resource_helper::generate_k1_candidates(msg.ran.tdd_ul_dl_cfg_common,
                                                                        msg.ran.init_bwp_builder.pucch.min_k1)),
  init_bwp_builder(msg.ran.init_bwp_builder),
  rrm_policy_members(msg.rrm_policy_members),
  cfra_enabled(msg.ran.init_bwp_builder.rach.cfra_enabled),
  // SSB derived params.
  ssb_case(band_helper::get_ssb_pattern(msg.ran.dl_carrier.band, msg.ran.ssb_cfg.scs)),
  paired_spectrum(band_helper::is_paired_spectrum(msg.ran.dl_carrier.band)),
  band(msg.ran.dl_carrier.band),
  L_max(ssb_get_L_max(msg.ran.ssb_cfg.scs, msg.ran.dl_carrier.arfcn_f_ref, msg.ran.dl_carrier.band)),
  ntn_cs_koffset(
      msg.ran.ntn_params.has_value()
          ? msg.ran.ntn_params->ntn_cfg.cell_specific_koffset.value_or(std::chrono::milliseconds{0}).count() *
                get_nof_slots_per_subframe(scs_common)
          : 0),
  ul_harq_mode_b(msg.ran.ntn_params.has_value() ? msg.ran.ntn_params->ul_harq_mode_b : false)
{
  // Initiate dedicated sched BWP configs.
  ded_bwp_res.emplace(to_bwp_id(0), pci, to_bwp_id(0), dl_cfg_common.init_dl_bwp, &msg.dl_bwp_ded);

  if (tdd_cfg_common.has_value()) {
    // Cache list of DL and UL slots in case of TDD
    const unsigned tdd_period_slots = nof_slots_per_tdd_period(*msg.ran.tdd_ul_dl_cfg_common);
    dl_symbols_per_slot_lst.resize(tdd_period_slots);
    ul_symbols_per_slot_lst.resize(tdd_period_slots);
    for (unsigned slot_period_idx = 0; slot_period_idx < dl_symbols_per_slot_lst.size(); ++slot_period_idx) {
      dl_symbols_per_slot_lst[slot_period_idx] = get_active_tdd_dl_symbols(*msg.ran.tdd_ul_dl_cfg_common,
                                                                           slot_period_idx,
                                                                           dl_cfg_common.init_dl_bwp.generic_params.cp)
                                                     .length();
      ul_symbols_per_slot_lst[slot_period_idx] = get_active_tdd_ul_symbols(*msg.ran.tdd_ul_dl_cfg_common,
                                                                           slot_period_idx,
                                                                           ul_cfg_common.init_ul_bwp.generic_params.cp)
                                                     .length();
    }
  }
}
