// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "sr_helper.h"

bool ocudu::sr_helper::is_sr_opportunity_slot(const pucch_config& pucch_cfg, slot_point sl_tx)
{
  // NOTE: This function is only used within the scheduler, where we assume the UE has a \c ul_config, a \c pucch_cfg
  // and a \c sr_res_list (verified by the scheduler validator).
  const auto& sr_resource_cfg_list = pucch_cfg.sr_res_list;

  for (const auto& sr_res : sr_resource_cfg_list) {
    // Check if this slot corresponds to an SR opportunity for the UE.
    if ((sl_tx - sr_res.offset).to_uint() % sr_periodicity_to_slot(sr_res.period) == 0) {
      return true;
    }
  }

  return false;
}
