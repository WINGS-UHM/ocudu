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
#include "ocudu/scheduler/config/ran_cell_config_helper.h"
#include "ocudu/scheduler/config/time_domain_resource_helper.h"

using namespace ocudu;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::vector<zp_csi_rs_resource> make_zp_csi_rs_list(const ran_cell_config& cfg)
{
  if (!cfg.init_bwp_builder.csi.has_value()) {
    return {};
  }
  const auto csi_helper = config_helpers::make_csi_meas_config_builder_params(cfg);
  return csi_helper::make_periodic_zp_csi_rs_resource_list(csi_helper);
}

static std::vector<nzp_csi_rs_resource> make_nzp_csi_rs_list(const ran_cell_config& cfg)
{
  if (!cfg.init_bwp_builder.csi.has_value()) {
    return {};
  }
  const auto csi_helper = config_helpers::make_csi_meas_config_builder_params(cfg);
  return csi_helper::make_nzp_csi_rs_resource_list(csi_helper);
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
  ded_pucch_resources(
      config_helpers::build_pucch_resource_list(msg.ran.init_bwp_builder.pucch.resources,
                                                msg.ran.ul_cfg_common.init_ul_bwp.generic_params.crbs.length())),
  zp_csi_rs_list(make_zp_csi_rs_list(msg.ran)),
  nzp_csi_rs_list(make_nzp_csi_rs_list(msg.ran)),
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
  {
    const auto                   rlm_cfg   = config_helpers::make_rlm_config(msg.ran);
    const auto                   pdsch_cfg = config_helpers::make_pdsch_config(msg.ran);
    const bwp_downlink_dedicated bwp_dl_ded{
        .pdcch_cfg = msg.ran.init_bwp_builder.pdcch_cfg, .pdsch_cfg = pdsch_cfg, .rlm_cfg = rlm_cfg};
    ded_bwp_res.emplace(to_bwp_id(0), pci, to_bwp_id(0), dl_cfg_common.init_dl_bwp, &bwp_dl_ded);
  }

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
