/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_high_ntn_config_translators.h"
#include "du_high_unit_cell_ntn_config.h"
#include "ocudu/du/du_high/du_qos_config.h"
#include "ocudu/du/du_high/du_srb_config.h"
#include "ocudu/ran/qos/five_qi.h"
#include "ocudu/rlc/rlc_config.h"

using namespace ocudu;

static void ntn_augment_rlc_config(const du_high_unit_cell_ntn_config& ntn_cfg, rlc_config& rlc)
{
  if (ntn_cfg.cell_specific_koffset.count() > 1000) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 4000);
  } else if (ntn_cfg.cell_specific_koffset.count() > 800) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 2000);
  } else if (ntn_cfg.cell_specific_koffset.count() > 500) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 2000);
  } else if (ntn_cfg.cell_specific_koffset.count() > 300) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 1000);
  } else if (ntn_cfg.cell_specific_koffset.count() > 200) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 800);
  } else if (ntn_cfg.cell_specific_koffset.count() > 100) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 400);
  } else if (ntn_cfg.cell_specific_koffset.count() > 50) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 200);
  } else if (ntn_cfg.cell_specific_koffset.count() > 10) {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 100);
  } else {
    rlc.am.tx.t_poll_retx = std::max(rlc.am.tx.t_poll_retx, 50);
  }
}

void ocudu::ntn_augment_du_srb_config(const du_high_unit_cell_ntn_config&     ntn_cfg,
                                      std::map<srb_id_t, odu::du_srb_config>& srb_cfgs)
{
  // NTN is enabled, so we need to augment the RLC parameters for the NTN cell.
  for (auto& srb : srb_cfgs) {
    ntn_augment_rlc_config(ntn_cfg, srb.second.rlc);
  }
}

void ocudu::ntn_augment_du_qos_config(const du_high_unit_cell_ntn_config&      ntn_cfg,
                                      std::map<five_qi_t, odu::du_qos_config>& qos_cfgs)
{
  // NTN is enabled, so we need to augment the QoS parameters for the NTN cell.
  for (auto& qos : qos_cfgs) {
    ntn_augment_rlc_config(ntn_cfg, qos.second.rlc);
  }
}
