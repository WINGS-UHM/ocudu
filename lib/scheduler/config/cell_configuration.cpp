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

using namespace ocudu;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

cell_configuration::cell_configuration(const scheduler_expert_config&                  expert_cfg_,
                                       const sched_cell_configuration_request_message& msg) :
  expert_cfg(expert_cfg_),
  cell_index(msg.cell_index),
  cell_group_index(msg.cell_group_index),
  pci(msg.pci),
  scs_common(msg.scs_common),
  nof_dl_prbs(get_max_Nprb(msg.dl_carrier.carrier_bw, scs_common, band_helper::get_freq_range(msg.dl_carrier.band))),
  nof_ul_prbs(get_max_Nprb(msg.ul_carrier.carrier_bw, scs_common, band_helper::get_freq_range(msg.dl_carrier.band))),
  nof_slots_per_frame(get_nof_slots_per_subframe(msg.dl_cfg_common.init_dl_bwp.generic_params.scs) *
                      NOF_SUBFRAMES_PER_FRAME),
  dl_cfg_common(msg.dl_cfg_common),
  ul_cfg_common(msg.ul_cfg_common),
  tdd_cfg_common(msg.tdd_ul_dl_cfg_common),
  dl_carrier(msg.dl_carrier),
  ssb_cfg(msg.ssb_config),
  dmrs_typeA_pos(msg.dmrs_typeA_pos),
  ul_carrier(msg.ul_carrier),
  coreset0(msg.coreset0),
  searchspace0(msg.searchspace0),
  init_bwp_res(pci, to_bwp_id(0), dl_cfg_common.init_dl_bwp, nullptr),
  ded_pucch_resources(msg.ded_pucch_resources),
  zp_csi_rs_list(msg.zp_csi_rs_list),
  nzp_csi_rs_list(msg.nzp_csi_rs_res_list),
  dl_data_to_ul_ack(msg.dl_data_to_ul_ack),
  init_bwp_builder(msg.init_bwp_builder),
  rrm_policy_members(msg.rrm_policy_members),
  cfra_enabled(msg.cfra_enabled),
  // SSB derived params.
  ssb_case(band_helper::get_ssb_pattern(msg.dl_carrier.band, msg.ssb_config.scs)),
  paired_spectrum(band_helper::is_paired_spectrum(msg.dl_carrier.band)),
  band(msg.dl_carrier.band),
  L_max(ssb_get_L_max(msg.ssb_config.scs, msg.dl_carrier.arfcn_f_ref, msg.dl_carrier.band)),
  ntn_cs_koffset(msg.ntn_cs_koffset),
  ul_harq_mode_b(msg.ul_harq_mode_b)
{
  // Initiate dedicated sched BWP configs.
  ded_bwp_res.emplace(to_bwp_id(0), pci, to_bwp_id(0), dl_cfg_common.init_dl_bwp, &msg.dl_bwp_ded);

  if (tdd_cfg_common.has_value()) {
    // Cache list of DL and UL slots in case of TDD
    const unsigned tdd_period_slots = nof_slots_per_tdd_period(*msg.tdd_ul_dl_cfg_common);
    dl_symbols_per_slot_lst.resize(tdd_period_slots);
    ul_symbols_per_slot_lst.resize(tdd_period_slots);
    for (unsigned slot_period_idx = 0; slot_period_idx < dl_symbols_per_slot_lst.size(); ++slot_period_idx) {
      dl_symbols_per_slot_lst[slot_period_idx] = get_active_tdd_dl_symbols(*msg.tdd_ul_dl_cfg_common,
                                                                           slot_period_idx,
                                                                           dl_cfg_common.init_dl_bwp.generic_params.cp)
                                                     .length();
      ul_symbols_per_slot_lst[slot_period_idx] = get_active_tdd_ul_symbols(*msg.tdd_ul_dl_cfg_common,
                                                                           slot_period_idx,
                                                                           ul_cfg_common.init_ul_bwp.generic_params.cp)
                                                     .length();
    }
  }
}
