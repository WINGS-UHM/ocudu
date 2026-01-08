/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "pucch_power_controller.h"
#include "../config/ue_configuration.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/power_control/tpc_mapping.h"
#include "ocudu/ran/pucch/pucch_info.h"
#include "ocudu/support/format/fmt_to_c_str.h"

using namespace ocudu;

pucch_power_controller::pucch_power_controller(const ue_cell_configuration& ue_cell_cfg,
                                               ocudulog::basic_logger&      logger_) :
  rnti(ue_cell_cfg.crnti),
  cl_pw_control_enabled(ue_cell_cfg.cell_cfg_common.expert_cfg.ue.ul_power_ctrl.enable_pucch_cl_pw_control),
  pucch_f0_sinr_target_dB(ue_cell_cfg.cell_cfg_common.expert_cfg.ue.ul_power_ctrl.pucch_f0_sinr_target_dB),
  pucch_f2_sinr_target_dB(ue_cell_cfg.cell_cfg_common.expert_cfg.ue.ul_power_ctrl.pucch_f2_sinr_target_dB),
  pucch_f3_sinr_target_dB(ue_cell_cfg.cell_cfg_common.expert_cfg.ue.ul_power_ctrl.pucch_f3_sinr_target_dB),
  tpc_adjust_prohibit_time_sl([&ue_cell_cfg]() -> unsigned {
    return tpc_adjust_prohibit_time_ms << to_numerology_value(
               ue_cell_cfg.cell_cfg_common.ul_cfg_common.init_ul_bwp.generic_params.scs);
  }()),
  pucch_f0_f1_sinr_dB(alpha_ema_sinr),
  pucch_f2_f3_f4_sinr_dB(alpha_ema_sinr),
  logger(logger_)
{
  // Save the PUCCH power control configuration.
  reconfigure(ue_cell_cfg);

  // Initialize PUCCH PRB grid.
  std::fill(pucch_pw_ctrl_grid.begin(), pucch_pw_ctrl_grid.end(), pucch_grants_pw_ctrl{});

  // Dummy casts only needed to prevent Clang from complaining about unused variables.
  static_cast<void>(rnti);
  static_cast<void>(cl_pw_control_enabled);
  static_cast<void>(pucch_f0_sinr_target_dB);
  static_cast<void>(pucch_f2_sinr_target_dB);
  static_cast<void>(pucch_f3_sinr_target_dB);
}

void pucch_power_controller::reconfigure(const ue_cell_configuration& ue_cell_cfg)
{
  if (ue_cell_cfg.init_bwp().ul_ded.has_value() and ue_cell_cfg.init_bwp().ul_ded->pucch_cfg.has_value()) {
    const auto& pucch_cfg = ue_cell_cfg.init_bwp().ul_ded->pucch_cfg.value();
    // Retrieve the Format of the resources in PUCCH set 0 and 1. NOTE: all resources are expected to be of the same
    // format.
    static constexpr size_t id_pucch_res_set_0 = 0U;
    unsigned pucch_res_set_0_id = pucch_cfg.pucch_res_set[id_pucch_res_set_0].pucch_res_id_list.front().ue_res_id;
    auto*    res_set_0          = std::find_if(
        pucch_cfg.pucch_res_list.begin(),
        pucch_cfg.pucch_res_list.end(),
        [pucch_res_set_0_id](const pucch_resource& res) { return res.res_id.ue_res_id == pucch_res_set_0_id; });
    ocudu_assert(res_set_0 != pucch_cfg.pucch_res_list.end(),
                 "rnti={}: PUCCH resource id={} not found in PUCCH resource set 0",
                 rnti,
                 pucch_res_set_0_id);
    format_set_0                               = res_set_0->format;
    static constexpr size_t id_pucch_res_set_1 = 1U;
    unsigned pucch_res_set_1_id = pucch_cfg.pucch_res_set[id_pucch_res_set_1].pucch_res_id_list.front().ue_res_id;
    auto*    res_set_1          = std::find_if(
        pucch_cfg.pucch_res_list.begin(),
        pucch_cfg.pucch_res_list.end(),
        [pucch_res_set_1_id](const pucch_resource& res) { return res.res_id.ue_res_id == pucch_res_set_1_id; });
    ocudu_assert(res_set_1 != pucch_cfg.pucch_res_list.end(),
                 "rnti={}: PUCCH resource id={} not found in PUCCH resource set 1",
                 rnti,
                 pucch_res_set_1_id);
    format_set_1 = res_set_1->format;
    if (ue_cell_cfg.init_bwp().ul_ded->pucch_cfg.value().pucch_pw_control.has_value()) {
      pucch_pwr_ctrl.emplace(ue_cell_cfg.init_bwp().ul_ded->pucch_cfg.value().pucch_pw_control.value());
    }
  }
}

static float compute_delta_tf_format_2_3_4(unsigned nof_uci_bits, unsigned payload_plus_crc_bits, unsigned n_res)
{
  static constexpr unsigned max_uci_payload_no_crc = 11U;
  // Number of bits per RE.
  const float bpre = static_cast<float>(payload_plus_crc_bits) / static_cast<float>(n_res);

  // Compute the delta TF value for the PUCCH power control as per Section 7.2.1, TS 38.213.
  return nof_uci_bits <= max_uci_payload_no_crc ? convert_power_to_dB(6.0f * bpre)
                                                : convert_power_to_dB(std::pow(2.0f, 2.4f * bpre) - 1.0);
}

// Compute the Delta TF \f$\Delta_{TF}\f$ value for the PUCCH power control as per Section 7.2.1, TS 38.213.
static float compute_delta_tf(pucch_format   format,
                              unsigned       nof_prbs,
                              unsigned       nof_symb,
                              pucch_uci_bits uci_bits,
                              bool           pi_2_bpsk,
                              bool           additional_dmrs,
                              bool           intraslot_freq_hopping)
{
  float delta_tf = 0.0f;
  // TODO: introduce a bias for the delta TF value based on the SINR targes given in Section 8.3, TS 38.104, where each
  //       SINR target is computed for specific number or RBs, symbols and UCI bits.
  switch (format) {
    case pucch_format::FORMAT_0:
      delta_tf = convert_power_to_dB(2.0f / static_cast<float>(nof_symb));
      break;
    case pucch_format::FORMAT_1: {
      // NOTE: To handle the case of PUCCH format 1 with only SR, when the UCI only contains the SR, we count 1 UCI bit.
      // This is because PUCCH format 1 with only SR uses BPSK modulation, the same as when PUCCH F1 transmits 1
      // HARQ bit (ref. to TS 38.211, Section 6.3.2.4.1).
      const unsigned uci_bits_pw_ctrl_f1 = uci_bits.harq_ack_nof_bits != 0 ? uci_bits.harq_ack_nof_bits : 1U;
      delta_tf                           = convert_power_to_dB(14.0f / static_cast<float>(nof_symb)) +
                 convert_power_to_dB(static_cast<float>(uci_bits_pw_ctrl_f1));
      break;
    }
    case pucch_format::FORMAT_2: {
      // As per Sections 6.3.1.2.1 and 6.3.1.4.1, TS 38.212, the parameter \f$E\f$ used to derive the number of
      // code-blocks is \f$E_{UCI}\f$.
      const unsigned nof_uci_bits          = uci_bits.get_total_bits();
      const unsigned e_uci                 = get_pucch_format2_E_total(nof_prbs, nof_symb);
      const unsigned payload_plus_crc_bits = nof_uci_bits + get_uci_nof_crc_bits(nof_uci_bits, e_uci);

      // Number of resource elements.
      const unsigned n_re = nof_prbs * nof_symb * pucch_constants::FORMAT2_NOF_DATA_SC;

      // For the following computation, refer to the relevant section for the used formula.
      delta_tf = compute_delta_tf_format_2_3_4(nof_uci_bits, payload_plus_crc_bits, n_re);
      break;
    }
    case pucch_format::FORMAT_3:
    case pucch_format::FORMAT_4: {
      const unsigned nof_uci_bits          = uci_bits.get_total_bits();
      const unsigned e_uci                 = format == ocudu::pucch_format::FORMAT_3
                                                 ? get_pucch_format3_E_total(nof_prbs, nof_symb, pi_2_bpsk)
                                                 : get_pucch_format4_E_total(nof_prbs, nof_symb, pi_2_bpsk);
      const unsigned payload_plus_crc_bits = nof_uci_bits + get_uci_nof_crc_bits(nof_uci_bits, e_uci);

      // Number of resource elements.
      const unsigned n_re =
          nof_prbs *
          (nof_symb - get_pucch_format3_4_nof_dmrs_symbols(nof_symb, intraslot_freq_hopping, additional_dmrs)) *
          NOF_SUBCARRIERS_PER_RB;

      delta_tf = compute_delta_tf_format_2_3_4(nof_uci_bits, payload_plus_crc_bits, n_re);
      break;
    }
    default:
      break;
  }

  return delta_tf;
}

uint8_t pucch_power_controller::get_tpc(float sinr_to_target_diff)
{
  static constexpr uint8_t default_tpc = 1;
  if (not latest_pucch_pw_control.has_value()) {
    return default_tpc;
  }

  // [Implementation-defined] We aim at keeping the power control adjustment within the limits given in \ref
  // g_cl_pw_control_bounds.
  const int max_g_cl_pw_control_increment =
      g_cl_pw_control_bounds.stop() - latest_pucch_pw_control.value().g_cl_pw_control;
  const int max_g_cl_pw_control_decrement =
      latest_pucch_pw_control.value().g_cl_pw_control - g_cl_pw_control_bounds.start();

  // [Implementation-defined] In the following, we compute the TPC command based on the SINR difference, to approximate
  // the mapping in Table 7.2.1-1, TS 38.213. The mapping we use is the following:
  // SINR diff. (dB) > 2.5dB -> delta_PUCCH = 3; TCP = 3.
  // 0.5dB < SINR diff. (dB) <= 2.5dB -> delta_PUCCH = 1; TCP = 2.
  // -0.5dB < SINR diff. (dB) <= 0.5dB -> delta_PUCCH = 0; TCP = 1.
  // SINR diff. (dB) < -0.5dB -> delta_PUCCH = -1; TCP = 0.
  // NOTE: we need also to ensure that the power control adjustment is within the limits given in
  // \ref g_cl_pw_control_bounds.
  uint8_t tpc_command = default_tpc;
  // We only allow an increase of 3 units of g_cl_pw_control_bounds if max_g_cl_pw_control_increment is not less than 3.
  if (sinr_to_target_diff > 2.5f and max_g_cl_pw_control_increment >= 3) {
    tpc_command = 3U;
  }
  // We only allow an increase of 1 unit of g_cl_pw_control_bounds if max_g_cl_pw_control_increment is not less than 1.
  else if (sinr_to_target_diff > 0.5f and max_g_cl_pw_control_increment >= 1) {
    tpc_command = 2U;
  }
  // We only allow a decrease of 1 unit of g_cl_pw_control_bounds if max_g_cl_pw_control_decrement is not less than 1.
  else if (sinr_to_target_diff <= -0.5f and max_g_cl_pw_control_decrement >= 1) {
    tpc_command = 0U;
  } else {
    tpc_command = 1U;
  }

  return tpc_command;
}

void pucch_power_controller::update_pucch_sinr_f0_f1(slot_point slor_rx, float sinr_db)
{
  if (not cl_pw_control_enabled) {
    return;
  }

  const auto& pucchs_pw_ctrl = pucch_pw_ctrl_grid[slor_rx.to_uint()];

  // For PUCCH format 1, there can be up to 2 PUCCH power control data, in case of PUCCH with HARQ-ACK and SR. However,
  // either PUCCH report the same delta TF value.
  // NOTE: The UE only transmit one of these PUCCHs.
  const auto& pucch_pw = std::find_if(pucchs_pw_ctrl.begin(), pucchs_pw_ctrl.end(), [](const auto& pucch) {
    return pucch.format == pucch_format::FORMAT_0 || pucch.format == pucch_format::FORMAT_1;
  });

  if (pucch_pw == pucchs_pw_ctrl.end()) {
    // TODO: only here for debugging purposes, consider removing this.
    fmt::memory_buffer fmtbuf;
    for (static_vector<pucch_pw_ctrl_data, MAX_SCHED_PUCCH_GRANTS_PER_UE>& grid_item : pucch_pw_ctrl_grid) {
      for (auto& item_item : grid_item) {
        if (item_item.slot_rx.valid()) {
          fmt::format_to(std::back_inserter(fmtbuf),
                         "{}[slot_rx={} grid_idx={} {} bits={}]",
                         fmtbuf.size() == 0 ? "" : ", ",
                         item_item.slot_rx,
                         item_item.slot_rx.to_uint() % pucch_pw_ctrl_grid.size(),
                         to_string(item_item.format),
                         item_item.uci_bits);
        }
      }
    }

    // This is not a critical event. The SINR update is trigger by a PUCCH F0/1 reception with valid UCI. To compute
    // the delta TF value for SINR, we need to know the PUCCH data that have been previously saved in the ring-buffer
    // grid, which ONLY happens after the UE leaves fallback. Therefore, some PUCCH might SINR updates might not have a
    // corresponding PUCCH power control data. Thus, the event below can occur in 2 cases: 1) At
    // RRC-Setup/Reconfiguration, 2) if the delay between the PUCCH tx slot and the slot at which the PUCCH is delivered
    // to the scheduler exceeds the grid size.
    logger.info("rnti={}: Power control data for PUCCH F0/1 not found for slot={} grid_idx={}. Currently "
                "the grid contains={}. Unless occurred during Setup/ReConf, consider increasing the grid size",
                rnti,
                slor_rx,
                slor_rx.to_uint() % pucch_pw_ctrl_grid.size(),
                to_c_str(fmtbuf));
    return;
  }

  // The SINR value is adjusted by the delta TF value. This is to avoid the SINR to oscillate based on deterministic
  // parameters known by the GNB, such as UCI bits and number of symbols.
  pucch_f0_f1_sinr_dB.push(static_cast<float>(sinr_db - pucch_pw->delta_tf));
}

void pucch_power_controller::update_pucch_sinr_f2_f3_f4(slot_point slor_rx,
                                                        float      sinr_db,
                                                        bool       has_harq_bits,
                                                        bool       has_csi_bits)
{
  if (not cl_pw_control_enabled) {
    return;
  }

  const auto& pucchs_pw_ctrl = pucch_pw_ctrl_grid[slor_rx.to_uint()];

  // For PUCCH format 2-3-4, there can be up to 2 PUCCH power control data. Can tell them apart depending on the UCI
  // bits.
  const auto& pucch_pw =
      std::find_if(pucchs_pw_ctrl.begin(), pucchs_pw_ctrl.end(), [has_harq_bits, has_csi_bits](const auto& pucch) {
        return (pucch.format == pucch_format::FORMAT_2 || pucch.format == pucch_format::FORMAT_3 ||
                pucch.format == pucch_format::FORMAT_4) and
               (has_harq_bits == (pucch.uci_bits.harq_ack_nof_bits > 0)) and
               (has_csi_bits == (pucch.uci_bits.csi_part1_nof_bits > 0));
      });

  if (pucch_pw == pucchs_pw_ctrl.end()) {
    // TODO: only here for debugging purposes, consider removing this.
    fmt::memory_buffer fmtbuf;
    for (static_vector<pucch_pw_ctrl_data, MAX_SCHED_PUCCH_GRANTS_PER_UE>& grid_item : pucch_pw_ctrl_grid) {
      for (auto& item_item : grid_item) {
        if (item_item.slot_rx.valid()) {
          fmt::format_to(std::back_inserter(fmtbuf),
                         "{}[slot_rx={} grid_idx={} {} bits={}]",
                         fmtbuf.size() == 0 ? "" : ", ",
                         item_item.slot_rx,
                         item_item.slot_rx.to_uint() % pucch_pw_ctrl_grid.size(),
                         to_string(item_item.format),
                         item_item.uci_bits);
        }
      }
    }

    // This is not a critical event. The SINR update is trigger by a PUCCH F0/1 reception with valid UCI. To compute the
    // delta TF value for SINR, we need to know the PUCCH data that have been previously saved in the ring-buffer grid,
    // which ONLY happens after the UE leaves fallback. Therefore, some PUCCH might SINR updates might not have a
    // corresponding PUCCH power control data. Thus, the event below can occur in 2 cases: 1) At
    // RRC-Setup/Reconfiguration, 2) if the delay between the PUCCH tx slot and the slot at which the PUCCH is delivered
    // to the scheduler exceeds the grid size.
    logger.info("rnti={}: Power control data for PUCCH F2/3/4 not found for slot={} grid_idx={}. Currently "
                "the grid contains={}. Unless occurred during Setup/ReConf, consider increasing the grid size",
                rnti,
                slor_rx,
                slor_rx.to_uint() % pucch_pw_ctrl_grid.size(),
                to_c_str(fmtbuf));

    return;
  }

  // The SINR value is adjusted by the delta TF value. This is to avoid the SINR to oscillate based on deterministic
  // parameters known by the GNB, such as UCI bits, number of PRBs and symbols.
  pucch_f2_f3_f4_sinr_dB.push(static_cast<float>(sinr_db - pucch_pw->delta_tf));
}

void pucch_power_controller::update_pucch_pw_ctrl_state(slot_point      slot,
                                                        pucch_format    format,
                                                        unsigned        nof_prbs,
                                                        unsigned        nof_symb,
                                                        pucch_uci_bits& uci_bits,
                                                        bool            intraslot_freq_hopping,
                                                        bool            pi_2_bpsk,
                                                        bool            additional_dmrs)
{
  if (not cl_pw_control_enabled) {
    return;
  }

  const unsigned grid_idx            = slot.to_uint();
  const auto&    slot_pucchs_pw_ctrl = pucch_pw_ctrl_grid[grid_idx];

  // If there are PUCCH power control data for a slot different from \ref slot, it means that this element of the ring
  // buffer contains old PUCCH power control data that are not needed anymore.
  // NOTE: It is possible that, due that this never gets cleared if no PUCCH is scheduled for a slot multiple times,
  // until, we have a slot wrap-around. Consider downgrading the log below to info.
  const bool clear_slot = std::any_of(slot_pucchs_pw_ctrl.begin(),
                                      slot_pucchs_pw_ctrl.end(),
                                      [slot](const auto& pucch) { return pucch.slot_rx != slot; });
  if (clear_slot) {
    pucch_pw_ctrl_grid[slot.to_uint()].clear();
  }

  if (pucch_pw_ctrl_grid[grid_idx].full()) {
    // This is not a critical event, it only causes the PUCCH closed-loop power control to be skipped for a given slot.
    logger.info("rnti={}: PUCCH power control grid is full at slot={} grid_idx={}. Cannot store more PUCCH power "
                "control data. This might have caused by a previous skipped slot",
                rnti,
                slot,
                grid_idx);
    return;
  }

  pucch_pw_ctrl_grid[grid_idx].emplace_back(
      slot,
      nof_prbs,
      format,
      uci_bits,
      compute_delta_tf(format, nof_prbs, nof_symb, uci_bits, pi_2_bpsk, additional_dmrs, intraslot_freq_hopping));
}

uint8_t pucch_power_controller::compute_tpc_command(slot_point pucch_slot)
{
  // The goal of the closed-loop power control for PUCCH is to adjust the transmit power to reach the target SINR.
  // However, we only do this for PUCCH format 0, 2 and 3. As Format 1 and Format 4 use Cyclic-shift and/or OCC, the
  // change in received power would not be reflected (or only marginally) in the SINR.
  // If PUCCH Format 1 is used, closed-loop power control only considers the SINR target for PUCCH Format 2 or 3.
  // If PUCCH Format 0 is used with Format 4, closed-loop power control only considers the SINR target for PUCCH Format
  // 0.
  // If PUCCH Format 0 is used with Format 2 or 3, then closed-loop power control should ensure that both SINR are above
  // the threshold.

  static constexpr uint8_t default_tpc = 1;
  if (not cl_pw_control_enabled or not pucch_pwr_ctrl.has_value()) {
    return default_tpc;
  }

  if (not latest_pucch_pw_control.has_value()) {
    static constexpr int default_f_cl_pw_control = 0;
    latest_pucch_pw_control.emplace(pucch_pw_control{default_f_cl_pw_control, pucch_slot});
    return default_tpc;
  } else if (pucch_slot <= latest_pucch_pw_control.value().latest_tpc_slot + tpc_adjust_prohibit_time_sl) {
    return default_tpc;
  }

  if (format_set_0 == pucch_format::NOF_FORMATS or format_set_1 == pucch_format::NOF_FORMATS or
      (format_set_0 == pucch_format::FORMAT_1 and format_set_1 == pucch_format::FORMAT_4)) {
    return default_tpc;
  }

  // This is only used for Format 2 and 3.
  const float pucch_f2_f3_target_sinr_dB =
      format_set_1 == pucch_format::FORMAT_2 ? pucch_f2_sinr_target_dB : pucch_f3_sinr_target_dB;
  const float sinr_to_target_f2_f3_diff = pucch_f2_f3_target_sinr_dB - pucch_f2_f3_f4_sinr_dB.average();

  // The PUCCH power control only considers the PUCCH formats 0, 2 and 3. As Format 1 and Format 4 use Cyclic-shift
  // and/or OCC, the change in received power would not be reflected (or only marginally) in the SINR.
  uint8_t tpc = default_tpc;
  if (format_set_0 == pucch_format::FORMAT_0) {
    const float sinr_to_target_f0_diff = pucch_f0_sinr_target_dB - pucch_f0_f1_sinr_dB.average();
    // If PUCCH Format 0 is used with Format 2 or 3, always considers the maximum of SINR differences; this should
    // ensure that both SINRs aren't below their respective targets and; if the SINR difference is negative for both
    // PUCCH Format 0 and Format 2/3 (both SINRs are above their SINR targets), then considering the maximum should
    // ensure that none of the SINRs will end up below their SINR targets.
    tpc = format_set_1 == pucch_format::FORMAT_4 ? get_tpc(sinr_to_target_f0_diff)
                                                 : get_tpc(std::max(sinr_to_target_f0_diff, sinr_to_target_f2_f3_diff));
  }

  if (format_set_0 == pucch_format::FORMAT_1 and
      (format_set_1 == pucch_format::FORMAT_2 or format_set_1 == pucch_format::FORMAT_3)) {
    tpc = get_tpc(sinr_to_target_f2_f3_diff);
  }

  latest_pucch_pw_control.value().g_cl_pw_control += tpc_mapping(tpc);
  latest_pucch_pw_control.value().latest_tpc_slot = pucch_slot;

  return tpc;
}
