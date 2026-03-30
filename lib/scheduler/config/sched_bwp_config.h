// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/pdcch/aggregation_level.h"
#include "ocudu/ran/pdcch/pdcch_constants.h"
#include "ocudu/scheduler/config/bwp_configuration.h"
#include "ocudu/scheduler/config/cell_bwp_res_config.h"
#include "ocudu/scheduler/config/ran_cell_config.h"
#include "ocudu/scheduler/config/serving_cell_config.h"

namespace ocudu {

/// List of CRBs for a given PDCCH candidate.
using crb_index_list      = static_vector<uint16_t, pdcch_constants::MAX_NOF_RB_PDCCH>;
using crb_index_list_span = span<const uint16_t>;

/// Parameters of a CORESET that are general to all the UEs served in a given BWP.
class sched_coreset_config
{
public:
  sched_coreset_config(pci_t pci, const bwp_configuration& bwp_cfg, const coreset_configuration& cs_cfg);

  /// CORESET identifier.
  coreset_id id() const { return cfg_ptr->get_id(); }

  /// Fetch CORESET configuration.
  const coreset_configuration& cfg() const { return *cfg_ptr; }

  /// Fetch list of CRBs for a given PDCCH candidate.
  crb_index_list_span candidate_crbs(uint8_t start_ncce, aggregation_level aggr_lvl) const
  {
    ocudu_sanity_check(start_ncce % to_nof_cces(aggr_lvl) == 0,
                       "The provided ncce is not aligned to the aggregation level");
    const unsigned ncce_idx = start_ncce >> to_aggregation_level_index(aggr_lvl); // ncce / L.
    return ncce_crbs[to_aggregation_level_index(aggr_lvl)][ncce_idx];
  }

  bool operator==(const coreset_configuration& cs_cfg) const { return *cfg_ptr == cs_cfg; }

private:
  /// Pointer to the CORESET configuration.
  const coreset_configuration* cfg_ptr = nullptr;

  /// Table of CRB lists for each NCCE and aggregation level.
  std::array<std::vector<crb_index_list>, NOF_AGGREGATION_LEVELS> ncce_crbs;
};

/// \brief Holds all the information respective to the configuration and management of a BWP from the scheduler
/// perspective.
///
/// This config must represent the superset of all the possible common and UE-dedicated configurations for a given BWP.
class sched_bwp_config
{
public:
  sched_bwp_config(const ran_cell_config& ran_cfg, bwp_id_t bwp_id_);

  bwp_id_t bwp_id() const { return bwpid; }

  /// BWP Downlink common config.
  const bwp_downlink_common& dl_common() const { return base_dl_bwp_cmn; }

  /// Fetch CORESET0 config if it exists.
  const sched_coreset_config* coreset0() const { return cs_list[0].id() == to_coreset_id(0) ? &cs_list[0] : nullptr; }

  /// List of CORESETs configured in the scheduler for this BWP.
  /// \note The index does not necessarily match the CORESET-ID.
  span<const sched_coreset_config> coresets() const { return cs_list; }

  /// Dedicated Downlink resources.
  const cell_dl_bwp_res_config& dl() const { return res.dl; }

  /// Dedicated Uplink resources.
  const cell_ul_bwp_res_config& ul() const { return res.ul; }

private:
  bwp_id_t            bwpid;
  bwp_downlink_common base_dl_bwp_cmn;

  cell_bwp_res_config res;

  std::vector<sched_coreset_config> cs_list;
};

} // namespace ocudu
