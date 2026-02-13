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

#include "../support/pdcch/search_space_helper.h"
#include "ocudu/adt/slotted_vector.h"
#include "ocudu/scheduler/scheduler_configurator.h"

namespace ocudu {

/// List of CRBs for a given PDCCH candidate.
using crb_index_list      = static_vector<uint16_t, pdcch_constants::MAX_NOF_RB_PDCCH>;
using crb_index_list_span = span<const uint16_t>;

/// Parameters of a CORESET that are general to all the UEs served in a given BWP.
class sched_coreset_config
{
public:
  sched_coreset_config(pci_t pci, const bwp_downlink_common& dl_bwp_cmn, const coreset_configuration& cs_cfg);

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

private:
  /// Pointer to the CORESET configuration.
  const coreset_configuration* cfg_ptr = nullptr;

  /// Table of CRB lists for each NCCE and aggregation level.
  std::array<std::vector<crb_index_list>, NOF_AGGREGATION_LEVELS> ncce_crbs;
};

/// \brief Holds all the information respective to the configuration and management of a BWP from the scheduler
/// perspective.
///
/// This config must represent the superset of all the possible UE-dedicated configurations for a given BWP.
class sched_bwp_config
{
public:
  sched_bwp_config(pci_t                         pci,
                   bwp_id_t                      bwp_id_,
                   const bwp_downlink_common&    base_dl_bwp_cmn_,
                   const bwp_downlink_dedicated* base_dl_bwp_);

  bwp_id_t bwp_id() const { return bwpid; }

  const bwp_downlink_common&                   dl_common() const { return base_dl_bwp_cmn; }
  const std::optional<bwp_downlink_dedicated>& dl_ded() const { return base_dl_bwp_ded; }

  /// List of CORESETs associated with this BWP.
  const slotted_id_vector<coreset_id, sched_coreset_config>& coresets() const { return cs_list; }

private:
  bwp_id_t                              bwpid;
  bwp_downlink_common                   base_dl_bwp_cmn;
  std::optional<bwp_downlink_dedicated> base_dl_bwp_ded;

  slotted_id_vector<coreset_id, sched_coreset_config> cs_list;
};

} // namespace ocudu
