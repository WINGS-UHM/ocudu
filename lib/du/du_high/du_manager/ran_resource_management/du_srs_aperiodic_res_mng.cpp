/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_srs_aperiodic_res_mng.h"
#include "du_ue_resource_config.h"
#include "ocudu/ran/srs/srs_bandwidth_configuration.h"
#include "ocudu/ran/srs/srs_configuration.h"

using namespace ocudu;
using namespace odu;

// Helper that computes the SRS bandwidth parameter \f$C_{SRS}\f$ based on the number of UL BWP RBs.
static std::optional<unsigned> compute_c_srs(unsigned nof_ul_bwp_rbs)
{
  // Iterate over Table 6.4.1.4.3-1, TS 38.211, and find the minimum \f$C_{SRS}\f$ value that maximizes \f$m_{SRS,0}\f$
  // under the constraint \f$m_{SRS,0}\f$ <= UL RBs.

  // As per Table 6.4.1.4.3-1, TS 38.211, the maximum value of \f$C_{SRS}\f$ is 63.
  constexpr unsigned max_non_valid_c_srs = 64;
  // Defines the pair of C_SRS and m_SRS values.
  using pair_c_srs_m_srs                          = std::pair<unsigned, unsigned>;
  std::optional<pair_c_srs_m_srs> candidate_c_srs = std::nullopt;
  for (uint8_t c_srs = 0; c_srs != max_non_valid_c_srs; ++c_srs) {
    // As per Table 6.4.1.4.3-1, TS 38.211, we do not consider frequency hopping.
    constexpr uint8_t b_srs_0    = 0;
    auto              srs_params = srs_configuration_get(c_srs, b_srs_0);

    if (not srs_params.has_value()) {
      ocudu_assertion_failure("C_SRS is not compatible with the current BW configuration");
      return std::nullopt;
    }

    // If there is no candidate C_SRS value, we set the first valid C_SRS value as the candidate.
    if (not candidate_c_srs.has_value()) {
      candidate_c_srs = pair_c_srs_m_srs{c_srs, srs_params.value().m_srs};
    }
    // NOTE: the condition srs_params.value().m_srs > candidate_c_srs->second is used to find the minimum C_SRS value
    // that maximizes m_SRS.
    else if (srs_params.value().m_srs <= nof_ul_bwp_rbs and srs_params.value().m_srs > candidate_c_srs->second) {
      candidate_c_srs = pair_c_srs_m_srs{c_srs, srs_params.value().m_srs};
    }
    // If we reach this point, no need to keep looking for a valid C_SRS value.
    if (srs_params.value().m_srs > nof_ul_bwp_rbs) {
      break;
    }
  }
  return candidate_c_srs.value().first;
}

// Helper that returns the PRB start value for the SRS within the UP BWP; the value is computed in such a way that the
// SRS resources are placed at the center of the BWP.
static unsigned compute_srs_rb_start(unsigned c_srs, unsigned nof_ul_bwp_rbs)
{
  // As per Section 6.4.1.4.3, the parameter \f$m_{SRS}\f$ = 0 is an index that, along with \f$C_{SRS}\f$, maps to the
  // bandwidth of the SRS resources.
  constexpr uint8_t                      b_srs_0    = 0;
  const std::optional<srs_configuration> srs_params = srs_configuration_get(c_srs, b_srs_0);
  ocudu_sanity_check(srs_params.has_value() and nof_ul_bwp_rbs >= srs_params.value().m_srs,
                     "The SRS configuration is not valid");

  return (nof_ul_bwp_rbs - srs_params.value().m_srs) / 2;
}

// Helper that updates the starting SRS config with user-defined parameters.
static srs_config build_default_srs_cfg(const du_cell_config& default_cell_cfg)
{
  ocudu_assert(default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.has_value() and
                   default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.has_value(),
               "DU cell config is not valid");

  ocudu_assert(not default_cell_cfg.srs_cfg.srs_period.has_value(),
               "Request to build aperiodic SRS configuration, but periodic parameters have been provided");

  // If SRS is not enabled, we don't need to update its configuration.
  if (not default_cell_cfg.srs_cfg.srs_enabled) {
    return default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.value();
  }

  auto srs_cfg = default_cell_cfg.ue_ded_serv_cell_cfg.ul_config.value().init_ul_bwp.srs_cfg.value();

  ocudu_assert(srs_cfg.srs_res_list.size() == 1 and srs_cfg.srs_res_set_list.size() == 1,
               "The SRS resource list and the SRS resource set list are expected to have a single element");

  srs_config::srs_resource& res = srs_cfg.srs_res_list.back();
  res.res_type                  = srs_resource_type::aperiodic;
  // Set the SRS resource ID to 0, as there is only 1 SRS resource per UE.
  res.id.ue_res_id = static_cast<srs_config::srs_res_id>(0U);

  srs_config::srs_resource_set& res_set = srs_cfg.srs_res_set_list.back();
  res_set.res_type.emplace<srs_config::srs_resource_set::aperiodic_resource_type>(
      srs_config::srs_resource_set::aperiodic_resource_type{});
  // Set the SRS resource set ID to 0, as there is only 1 SRS resource set per UE.
  res_set.id = static_cast<srs_config::srs_res_set_id>(0U);

  return srs_cfg;
}

static std::array<unsigned, 3> compute_slot_offsets(const du_cell_config& cell_cfg)
{
  if (not cell_cfg.tdd_ul_dl_cfg_common.has_value()) {
    return {1, 2, 3};
  }

  return {1, 2, 3};
}

du_srs_aperiodic_res_mng::cell_context::cell_context(const du_cell_config& cfg) :
  cell_cfg(cfg), tdd_ul_dl_cfg_common(cfg.tdd_ul_dl_cfg_common), default_srs_cfg(build_default_srs_cfg(cfg))
{
}

du_srs_aperiodic_res_mng::du_srs_aperiodic_res_mng(span<const du_cell_config> cell_cfg_list_) :
  cells(cell_cfg_list_.begin(), cell_cfg_list_.end())
{
  for (auto& cell : cells) {
    if (not cell.cell_cfg.srs_cfg.srs_period.has_value()) {
      continue;
    }

    // If the C_SRS is not set as an input parameter, then we compute C_SRS so that the SRS uses the maximum allowed
    // number of RBs and is located at the center of the UL BWP.
    if (cell.cell_cfg.srs_cfg.c_srs.has_value()) {
      cell.srs_common_params.c_srs      = cell.cell_cfg.srs_cfg.c_srs.value();
      cell.srs_common_params.freq_shift = cell.cell_cfg.srs_cfg.freq_domain_shift.value();
    } else {
      const std::optional<unsigned> c_srs =
          compute_c_srs(cell.cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length());
      ocudu_assert(c_srs.has_value(), "SRS parameters didn't provide a valid C_SRS value");
      cell.srs_common_params.c_srs = c_srs.value();
      // When computed automatically, \c freqDomainShift is set so that the SRS is placed at the center of the UL BWP.
      // As per TS 38.211, Section 6.4.1.4.3, if \f$n_{shift} >= BWP_RB_start\f$, the reference point for the SRS
      // subcarriers is the CRB idx 0, else it's the BWP_RB_start; in here, we implicitly assume \f$n_{shift} >=
      // BWP_RB_start\f$.
      cell.srs_common_params.freq_shift =
          compute_srs_rb_start(c_srs.value(), cell.cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.length()) +
          cell.cell_cfg.ul_cfg_common.init_ul_bwp.generic_params.crbs.start();
    }

    cell.srs_common_params.p0 = cell.cell_cfg.srs_cfg.p0;

    // TODO: evaluate whether we need to consider the case of multiple cells.
    cell.cell_srs_res_list = generate_cell_srs_list(cell.cell_cfg);

    const auto srs_period_slots = static_cast<unsigned>(cell.cell_cfg.srs_cfg.srs_period.value());
    // Reserve the size of the vector and set the SRS counter of each offset to 0.
    cell.srs_res_usage.reserve(cell.cell_srs_res_list.size());
    cell.srs_res_usage.assign(srs_period_slots, 0U);
  }
}

bool du_srs_aperiodic_res_mng::alloc_resources(cell_group_config& cell_grp_cfg)
{
  // From this point on, the allocation is expected to succeed, as there are SRS resources available in each cell.
  for (auto& cell_cfg_ded : cell_grp_cfg.cells) {
    auto& ue_du_cell = cells[cell_cfg_ded.serv_cell_cfg.cell_index];

    // The UE SRS configuration is taken from a base configuration, saved in the GNB. The UE specific parameters will be
    // added later on in this function.
    // NOTE: This config could be as well for non-periodic SRS, therefore emplace anyway.
    cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.emplace(ue_du_cell.default_srs_cfg);

    srs_config& ue_srs_cfg = cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.value();

    // Find the best resource ID and offset for this UE, according to the class policy.
    const auto opt_srs_res_it = ue_du_cell.find_optimal_ue_srs_resource();
    ocudu_assert(opt_srs_res_it != ue_du_cell.srs_res_usage.end(), "No SRS resource returned from a non-emtpy set");

    auto opt_res_idx = std::distance(ue_du_cell.srs_res_usage.cbegin(), opt_srs_res_it);

    const auto& du_res_it = ue_du_cell.get_du_srs_res_cfg(static_cast<unsigned>(opt_res_idx));
    ocudu_assert(du_res_it != ue_du_cell.cell_srs_res_list.end(), "The provided cell-ID is invalid");

    const auto& du_res = *du_res_it;

    // Update the SRS configuration with the parameters that are specific to this resource and for this UE.
    auto& only_ue_srs_res = ue_srs_cfg.srs_res_list.front();
    // NOTE: given that there is only 1 SRS resource per UE, we can assume that the SRS resource ID is 0.

    ue_du_cell.fill_srs_res_parameters(only_ue_srs_res, du_res);

    ue_du_cell.fill_srs_res_sets(
        ue_srs_cfg.srs_res_set_list, only_ue_srs_res.id.ue_res_id, compute_slot_offsets(ue_du_cell.cell_cfg));

    // Update the counter of UEs using this resource.
    ++ue_du_cell.srs_res_usage[opt_res_idx];
  }

  return true;
}

std::vector<unsigned>::const_iterator du_srs_aperiodic_res_mng::cell_context::find_optimal_ue_srs_resource()
{
  return std::min_element(
      srs_res_usage.begin(), srs_res_usage.end(), [](const unsigned lhs, const unsigned rhs) { return lhs < rhs; });
}

void du_srs_aperiodic_res_mng::cell_context::fill_srs_res_parameters(srs_config::srs_resource& res_out,
                                                                     const du_srs_resource&    res_in) const
{
  // NOTE: given that there is only 1 SRS resource per UE, we can assume that the SRS resource ID is 0.
  res_out.id.cell_res_id = res_in.cell_res_id;
  res_out.id.ue_res_id   = static_cast<srs_config::srs_res_id>(0U);
  ocudu_assert(cell_cfg.ul_carrier.nof_ant == 1 or cell_cfg.ul_carrier.nof_ant == 2 or
                   cell_context::cell_cfg.ul_carrier.nof_ant == 4,
               "The number of UL antenna ports is not valid");
  res_out.nof_ports                    = srs_config::srs_resource::nof_srs_ports::port1;
  res_out.tx_comb.size                 = cell_cfg.srs_cfg.tx_comb;
  res_out.tx_comb.tx_comb_offset       = res_in.tx_comb_offset.value();
  res_out.tx_comb.tx_comb_cyclic_shift = res_in.cs;
  res_out.freq_domain_pos              = res_in.freq_dom_position;
  res_out.res_mapping.start_pos        = NOF_OFDM_SYM_PER_SLOT_NORMAL_CP - res_in.symbols.start() - 1;
  res_out.res_mapping.nof_symb         = static_cast<srs_nof_symbols>(res_in.symbols.length());
  res_out.sequence_id                  = res_in.sequence_id;

  // Update the SRS configuration with the parameters that are common to the cell.
  res_out.freq_hop.c_srs = srs_common_params.c_srs;
  // We assume that the frequency hopping is disabled and that the SRS occupies all possible RBs within the BWP. Refer
  // to Section 6.4.1.4.3, TS 38.211.
  res_out.freq_hop.b_srs    = 0U;
  res_out.freq_hop.b_hop    = 0U;
  res_out.freq_domain_shift = srs_common_params.freq_shift;
}

void du_srs_aperiodic_res_mng::cell_context::fill_srs_res_sets(srs_set_t&             srs_res_set_list,
                                                               srs_config::srs_res_id res_id,
                                                               span<const unsigned>   slot_offsets) const
{
  ocudu_assert(slot_offsets.size() == 3, "Invalid number of slot_offsets");

  // Update the parameters.
  auto& srs_res_set = srs_res_set_list.front();
  srs_res_set.p0    = srs_common_params.p0;
  srs_res_set_list.emplace_back(srs_res_set);

  // The basic config has 1 SRS resource set; we expand it to 3 element, by copying the first one and then updating the
  // res set ID, slot_offset and code trigger.
  for (auto it = slot_offsets.begin(); it != slot_offsets.end(); ++it) {
    unsigned idx = std::distance(slot_offsets.begin(), it);

    if (it != slot_offsets.begin()) {
      srs_res_set_list.emplace_back(srs_res_set);
    }
    auto& srs_set = srs_res_set_list.back();
    // Index the SRS resource set ID consecutively from 0 to 3.
    srs_set.id               = static_cast<srs_config::srs_res_set_id>(idx);
    auto& aperiodic_set_type = std::get<srs_config::srs_resource_set::aperiodic_resource_type>(srs_set.res_type);
    aperiodic_set_type.slot_offset.emplace(*it);
    // We use the following map to activate the SRS sets (ref to Table 7.3.1.1.2-24, TS 38.212).
    // SRS resource set 0 -> aperiodic_srs_res_trigger 1.
    // SRS resource set 1 -> aperiodic_srs_res_trigger 2.
    // SRS resource set 2 -> aperiodic_srs_res_trigger 3.
    aperiodic_set_type.aperiodic_srs_res_trigger = static_cast<uint8_t>(srs_res_set_list.back().id) + 1U;
  }

  // Update the SRS resource set with the p0.
}

void du_srs_aperiodic_res_mng::dealloc_resources(cell_group_config& cell_grp_cfg)
{
  for (auto& cell_cfg_ded : cell_grp_cfg.cells) {
    // This is the cell index inside the DU.
    auto& ue_du_cell = cells[cell_cfg_ded.serv_cell_cfg.cell_index];

    if (not cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.has_value()) {
      continue;
    }

    const auto& ue_srs_cfg = cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.value();

    for (const auto& srs_res : ue_srs_cfg.srs_res_list) {
      const unsigned res_id_to_deallocate = srs_res.id.cell_res_id;

      ocudu_assert(ue_du_cell.srs_res_usage[srs_res.id.cell_res_id] < ue_du_cell.srs_res_usage.size(),
                   "The slot resource counter is expected to be non-zero");
      // Update the used_not_full slot vector.gnb
      ocudu_assert(ue_du_cell.srs_res_usage[srs_res.id.cell_res_id] != 0,
                   "The slot resource counter is expected to be non-zero");
      --ue_du_cell.srs_res_usage[srs_res.id.cell_res_id];
    }

    // Reset the SRS configuration in this UE. This makes sure the DU will exit this function immediately when it gets
    // called again for the same UE (upon destructor's call).
    cell_cfg_ded.serv_cell_cfg.ul_config->init_ul_bwp.srs_cfg.reset();
  }
}
