// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "sched_coreset_config.h"
#include "ocudu/adt/slotted_vector.h"
#include "ocudu/scheduler/config/bwp_configuration.h"
#include "ocudu/scheduler/config/cell_bwp_res_config.h"
#include "ocudu/scheduler/config/ran_cell_config.h"

namespace ocudu {

/// \brief Holds all the information respective to the configuration and management of a BWP from the scheduler
/// perspective.
///
/// This config must represent the superset of all the possible common and UE-dedicated configurations for a given BWP.
class sched_bwp_res_config
{
public:
  sched_bwp_res_config(const ran_cell_config& ran_cfg, bwp_id_t bwp_id_);

  bwp_id_t bwp_id() const { return bwpid; }

  /// BWP Downlink common config.
  const bwp_downlink_common& dl_common() const { return base_dl_bwp_cmn; }

  /// List of all possible CORESET configurations supported for this BWP.
  const slotted_id_vector<coreset_id, sched_coreset_config>& coresets() const { return cs_list; }

  /// Dedicated Downlink resources.
  const cell_dl_bwp_res_config& dl() const { return res.dl; }

  /// Dedicated Uplink resources.
  const cell_ul_bwp_res_config& ul() const { return res.ul; }

private:
  bwp_id_t            bwpid;
  bwp_downlink_common base_dl_bwp_cmn;

  cell_bwp_res_config res;

  slotted_id_vector<coreset_id, sched_coreset_config> cs_list;
};

} // namespace ocudu
