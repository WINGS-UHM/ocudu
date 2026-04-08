// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "tests/unittests/scheduler/test_utils/scheduler_test_suite.h"
#include "lib/scheduler/cell/resource_grid.h"
#include "lib/scheduler/common_scheduling/ra_scheduler.h"
#include "lib/scheduler/support/config_helpers.h"
#include "lib/scheduler/support/pdsch/pdsch_default_time_allocation.h"
#include "lib/scheduler/support/sched_result_helpers.h"
#include "scheduler_output_test_helpers.h"
#include "tests/test_doubles/scheduler/scheduler_test_message_validators.h"
#include "ocudu/adt/static_vector.h"
#include "ocudu/ran/band_helper.h"
#include "ocudu/ran/pdcch/dci_packing.h"
#include "ocudu/ran/prach/prach_configuration.h"
#include "ocudu/ran/prach/prach_cyclic_shifts.h"
#include "ocudu/ran/prach/ra_helper.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/resource_allocation_frequency.h"
#include "ocudu/scheduler/result/pucch_format.h"
#include "ocudu/scheduler/scheduler_feedback_handler.h"
#include "ocudu/support/error_handling.h"
#include <gtest/gtest.h>

using namespace ocudu;

void ocudu::assert_tdd_pattern_consistency(const cell_configuration& cell_cfg,
                                           slot_point                sl_tx,
                                           const sched_result&       result)
{
  if (not cell_cfg.is_tdd()) {
    return;
  }
  ofdm_symbol_range dl_symbols = get_active_tdd_dl_symbols(
      *cell_cfg.params.tdd_cfg, sl_tx.slot_index(), cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params.cp);
  ASSERT_EQ(dl_symbols.length(), result.dl.nof_dl_symbols);

  if (dl_symbols.empty()) {
    ASSERT_TRUE(result.dl.dl_pdcchs.empty());
    ASSERT_TRUE(result.dl.ul_pdcchs.empty());
    ASSERT_TRUE(result.dl.bc.ssb_info.empty());
    ASSERT_TRUE(result.dl.bc.sibs.empty());
    ASSERT_TRUE(result.dl.rar_grants.empty());
    ASSERT_TRUE(result.dl.paging_grants.empty());
    ASSERT_TRUE(result.dl.ue_grants.empty());
    ASSERT_TRUE(result.dl.csi_rs.empty());
  } else if (dl_symbols.length() != get_nsymb_per_slot(cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params.cp)) {
    // Partial slot case.
    for (const auto& ssb : result.dl.bc.ssb_info) {
      ASSERT_TRUE(dl_symbols.contains(ssb.symbols));
    }
    for (const auto& sib : result.dl.bc.sibs) {
      ASSERT_TRUE(dl_symbols.contains(sib.pdsch_cfg.symbols));
    }
    for (const auto& rar : result.dl.rar_grants) {
      ASSERT_TRUE(dl_symbols.contains(rar.pdsch_cfg.symbols));
    }
    for (const auto& paging_grant : result.dl.paging_grants) {
      ASSERT_TRUE(dl_symbols.contains(paging_grant.pdsch_cfg.symbols));
    }
    for (const auto& ue_grant : result.dl.ue_grants) {
      ASSERT_TRUE(dl_symbols.contains(ue_grant.pdsch_cfg.symbols));
    }
    for (const auto& csi_rs : result.dl.csi_rs) {
      ASSERT_TRUE(dl_symbols.contains(csi_rs.symbol0));
    }
  }

  ofdm_symbol_range ul_symbols = get_active_tdd_ul_symbols(
      *cell_cfg.params.tdd_cfg, sl_tx.to_uint(), cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.cp);
  ASSERT_EQ(ul_symbols.length(), result.ul.nof_ul_symbols);

  if (ul_symbols.empty()) {
    ASSERT_TRUE(result.ul.puschs.empty());
    ASSERT_TRUE(result.ul.prachs.empty());
    ASSERT_TRUE(result.ul.pucchs.empty());
    ASSERT_TRUE(result.ul.srss.empty());
  } else if (dl_symbols.length() != get_nsymb_per_slot(cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.cp)) {
    for (const auto& ue_grant : result.ul.puschs) {
      ASSERT_TRUE(ul_symbols.contains(ue_grant.pusch_cfg.symbols));
    }
    for (const auto& prach : result.ul.prachs) {
      ofdm_symbol_range prach_symbols = {prach.start_symbol, prach.start_symbol + get_preamble_duration(prach.format)};
      ASSERT_TRUE(ul_symbols.contains(prach_symbols));
    }
    for (const auto& pucch : result.ul.pucchs) {
      ASSERT_TRUE(ul_symbols.contains(pucch.resources.symbols));
    }
    for (const auto& srs : result.ul.srss) {
      ASSERT_TRUE(ul_symbols.contains(srs.symbols));
    }
  }
}

void ocudu::assert_pdcch_pdsch_common_consistency(const cell_configuration&   cell_cfg,
                                                  const pdcch_dl_information& pdcch,
                                                  const pdsch_information&    pdsch)
{
  ASSERT_EQ(pdcch.ctx.rnti, pdsch.rnti);
  ASSERT_TRUE(*pdcch.ctx.bwp_cfg == *pdsch.bwp_cfg);
  ASSERT_TRUE(*pdcch.ctx.coreset_cfg == *pdsch.coreset_cfg);
  bwp_configuration bwp_cfg = cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params;
  // See TS 38.214, 5.1.2.2.2, Downlink resource allocation type 1.
  if (cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common.coreset0.has_value()) {
    bwp_cfg.crbs = get_coreset0_crbs(cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common);
  }
  const crb_interval cs_zero_crbs = get_coreset0_crbs(cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common);

  unsigned                                          time_assignment = 0;
  unsigned                                          freq_assignment = 0;
  unsigned                                          N_rb_dl_bwp     = 0;
  span<const pdsch_time_domain_resource_allocation> td_list;
  switch (pdcch.dci.type) {
    case dci_dl_rnti_config_type::si_f1_0: {
      ASSERT_EQ(pdcch.ctx.rnti, rnti_t::SI_RNTI);
      time_assignment = pdcch.dci.si_f1_0.time_resource;
      freq_assignment = pdcch.dci.si_f1_0.frequency_resource;
      N_rb_dl_bwp     = pdcch.dci.si_f1_0.N_rb_dl_bwp;
      td_list         = get_si_rnti_pdsch_time_domain_list(cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params.cp,
                                                   cell_cfg.params.dmrs_typeA_pos);
      ASSERT_EQ(N_rb_dl_bwp, cs_zero_crbs.length());
      break;
    }
    case dci_dl_rnti_config_type::ra_f1_0: {
      time_assignment = pdcch.dci.ra_f1_0.time_resource;
      ASSERT_TRUE(time_assignment < cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list.size());
      freq_assignment = pdcch.dci.ra_f1_0.frequency_resource;
      N_rb_dl_bwp     = pdcch.dci.ra_f1_0.N_rb_dl_bwp;
      td_list         = get_ra_rnti_pdsch_time_domain_list(cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common,
                                                   cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params.cp,
                                                   cell_cfg.params.dmrs_typeA_pos);
      ASSERT_EQ(N_rb_dl_bwp, bwp_cfg.crbs.length());
      ASSERT_EQ(pdcch.ctx.context.ss_id, cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common.ra_search_space_id);
    } break;
    case dci_dl_rnti_config_type::tc_rnti_f1_0: {
      time_assignment = pdcch.dci.tc_rnti_f1_0.time_resource;
      freq_assignment = pdcch.dci.tc_rnti_f1_0.frequency_resource;
      N_rb_dl_bwp     = pdcch.dci.tc_rnti_f1_0.N_rb_dl_bwp;
      td_list         = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
      ASSERT_EQ(N_rb_dl_bwp, cs_zero_crbs.length());
    } break;
    case dci_dl_rnti_config_type::c_rnti_f1_0: {
      time_assignment = pdcch.dci.c_rnti_f1_0.time_resource;
      freq_assignment = pdcch.dci.c_rnti_f1_0.frequency_resource;
      N_rb_dl_bwp     = cs_zero_crbs.length();
      td_list         = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
    } break;
    case dci_dl_rnti_config_type::p_rnti_f1_0: {
      time_assignment = pdcch.dci.p_rnti_f1_0.time_resource;
      freq_assignment = pdcch.dci.p_rnti_f1_0.frequency_resource;
      N_rb_dl_bwp     = pdcch.dci.p_rnti_f1_0.N_rb_dl_bwp;
      td_list         = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
      ASSERT_EQ(N_rb_dl_bwp, cs_zero_crbs.length());
    } break;
    default:
      ocudu_terminate("DCI type not supported");
  }
  ASSERT_LT(time_assignment, td_list.size());
  ofdm_symbol_range symbols = td_list[time_assignment].symbols;
  ASSERT_EQ(symbols, pdsch.symbols) << "Mismatch of time-domain resource assignment and PDSCH symbols";

  unsigned pdsch_freq_resource = ra_frequency_type1_get_riv(
      ra_frequency_type1_configuration{N_rb_dl_bwp, pdsch.rbs.type1().start(), pdsch.rbs.type1().length()});
  ASSERT_EQ(pdsch_freq_resource, freq_assignment) << "DCI frequency resource does not match PDSCH PRBs";
}

void ocudu::assert_pdcch_pdsch_common_consistency(const cell_configuration&      cell_cfg,
                                                  const cell_resource_allocator& cell_res_grid)
{
  span<const pdcch_dl_information> pdcchs = cell_res_grid[0].result.dl.dl_pdcchs;
  for (const pdcch_dl_information& pdcch : pdcchs) {
    const pdsch_information* linked_pdsch = nullptr;
    switch (pdcch.dci.type) {
      case dci_dl_rnti_config_type::si_f1_0: {
        const auto&     sibs = cell_res_grid[0].result.dl.bc.sibs;
        sib_information sib;
        const auto&     it = std::find_if(sibs.begin(), sibs.end(), [&pdcch](const auto& sib_) {
          unsigned pdsch_freq_resource = ra_frequency_type1_get_riv(ra_frequency_type1_configuration{
              pdcch.dci.si_f1_0.N_rb_dl_bwp, sib_.pdsch_cfg.rbs.type1().start(), sib_.pdsch_cfg.rbs.type1().length()});
          return (sib_.pdsch_cfg.rnti == pdcch.ctx.rnti) &&
                 (pdsch_freq_resource == pdcch.dci.si_f1_0.frequency_resource);
        });
        ASSERT_NE(it, sibs.end());
        linked_pdsch = &it->pdsch_cfg;
      } break;
      case dci_dl_rnti_config_type::ra_f1_0: {
        uint8_t k0 =
            cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list[pdcch.dci.ra_f1_0.time_resource]
                .k0;
        const auto& rars = cell_res_grid[k0].result.dl.rar_grants;
        const auto* it   = std::find_if(
            rars.begin(), rars.end(), [&pdcch](const auto& rar) { return rar.pdsch_cfg.rnti == pdcch.ctx.rnti; });
        ASSERT_NE(it, rars.end());
        linked_pdsch = &it->pdsch_cfg;
      } break;
      case dci_dl_rnti_config_type::c_rnti_f1_0: {
        uint8_t k0 = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common
                         .pdsch_td_alloc_list[pdcch.dci.c_rnti_f1_0.time_resource]
                         .k0;
        const auto& ue_grants = cell_res_grid[k0].result.dl.ue_grants;
        const auto* it        = std::find_if(ue_grants.begin(), ue_grants.end(), [&pdcch](const auto& grant) {
          return grant.pdsch_cfg.rnti == pdcch.ctx.rnti;
        });
        ASSERT_NE(it, ue_grants.end());
        linked_pdsch = &it->pdsch_cfg;
      } break;
      case dci_dl_rnti_config_type::tc_rnti_f1_0: {
        uint8_t k0 = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common
                         .pdsch_td_alloc_list[pdcch.dci.tc_rnti_f1_0.time_resource]
                         .k0;
        const auto& ue_grants = cell_res_grid[k0].result.dl.ue_grants;
        const auto* it        = std::find_if(ue_grants.begin(), ue_grants.end(), [&pdcch](const auto& grant) {
          return grant.pdsch_cfg.rnti == pdcch.ctx.rnti;
        });
        ASSERT_NE(it, ue_grants.end());
        linked_pdsch = &it->pdsch_cfg;
      } break;
      case dci_dl_rnti_config_type::p_rnti_f1_0: {
        // No corresponding PDSCH.
        if (pdcch.dci.p_rnti_f1_0.short_messages_indicator ==
            dci_1_0_p_rnti_configuration::payload_info::short_messages) {
          break;
        }
        uint8_t k0 = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common
                         .pdsch_td_alloc_list[pdcch.dci.p_rnti_f1_0.time_resource]
                         .k0;
        const auto& pg_grants = cell_res_grid[k0].result.dl.paging_grants;
        const auto* it        = std::find_if(pg_grants.begin(), pg_grants.end(), [&pdcch](const auto& grant) {
          return grant.pdsch_cfg.rnti == pdcch.ctx.rnti;
        });
        ASSERT_NE(it, pg_grants.end());
        linked_pdsch = &it->pdsch_cfg;
      } break;
      default:
        ocudu_terminate("DCI type not supported");
    }
    if (linked_pdsch) {
      assert_pdcch_pdsch_common_consistency(cell_cfg, pdcch, *linked_pdsch);
    }
  }
}

void ocudu::test_pdsch_sib_consistency(const cell_configuration& cell_cfg, span<const sib_information> sibs)
{
  bool has_coreset0 = cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common.coreset0.has_value();
  if (not has_coreset0) {
    ASSERT_TRUE(sibs.empty()) << fmt::format("SIB1 cannot be scheduled without CORESET#0");
    return;
  }

  bwp_configuration effective_init_bwp_cfg = cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params;
  effective_init_bwp_cfg.crbs              = get_coreset0_crbs(cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common);

  for (const sib_information& sib : sibs) {
    ASSERT_EQ(sib.pdsch_cfg.rnti, rnti_t::SI_RNTI);
    ASSERT_EQ(sib.pdsch_cfg.dci_fmt, dci_dl_format::f1_0);
    ASSERT_TRUE(sib.pdsch_cfg.rbs.is_type1());
    ASSERT_EQ(sib.pdsch_cfg.coreset_cfg->get_id(), to_coreset_id(0));
    if (sib.si_indicator == sib_information::sib1) {
      ASSERT_EQ(sib.pdsch_cfg.ss_set_type, search_space_set_type::type0);
      ASSERT_FALSE(sib.si_msg_index.has_value());
    } else {
      ASSERT_TRUE(sib.pdsch_cfg.ss_set_type == search_space_set_type::type0 or
                  sib.pdsch_cfg.ss_set_type == search_space_set_type::type0A);
      ASSERT_TRUE(sib.si_msg_index.has_value());
    }
    ASSERT_EQ(sib.pdsch_cfg.codewords.size(), 1);
    ASSERT_EQ(sib.pdsch_cfg.codewords[0].mcs_table, pdsch_mcs_table::qam64);
    vrb_interval vrbs = sib.pdsch_cfg.rbs.type1();
    ASSERT_LE(vrbs.stop(), effective_init_bwp_cfg.crbs.length())
        << fmt::format("PRB grant falls outside CORESET#0 RB boundaries");
  }
}

void ocudu::test_pdsch_rar_consistency(const cell_configuration& cell_cfg, span<const rar_information> rars)
{
  std::set<rnti_t>                  ra_rntis;
  std::set<rnti_t>                  tc_rntis;
  const search_space_configuration& ss_cfg =
      cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common
          .search_spaces[cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common.ra_search_space_id];
  crb_interval      coreset0_lims = get_coreset0_crbs(cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common);
  bwp_configuration init_bwp_cfg  = cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params;

  for (const rar_information& rar : rars) {
    rnti_t ra_rnti = rar.pdsch_cfg.rnti;
    ASSERT_FALSE(rar.grants.empty()) << fmt::format("RAR with RA-rnti={} has no corresponding MSG3 grants", ra_rnti);
    ASSERT_EQ(rar.pdsch_cfg.dci_fmt, dci_dl_format::f1_0);
    ASSERT_TRUE(rar.pdsch_cfg.rbs.is_type1()) << "Invalid allocation type for RAR";
    ASSERT_EQ(rar.pdsch_cfg.coreset_cfg->get_id(), ss_cfg.get_coreset_id());
    ASSERT_EQ(rar.pdsch_cfg.ss_set_type, search_space_set_type::type1);
    ASSERT_EQ(rar.pdsch_cfg.codewords.size(), 1);
    ASSERT_EQ(rar.pdsch_cfg.codewords[0].mcs_table, pdsch_mcs_table::qam64);

    const prb_interval rar_vrbs = {
        rar.pdsch_cfg.rbs.type1().start() + rar.pdsch_cfg.coreset_cfg->get_coreset_start_crb(),
        rar.pdsch_cfg.rbs.type1().stop() + rar.pdsch_cfg.coreset_cfg->get_coreset_start_crb()};
    crb_interval rar_crbs = prb_to_crb(init_bwp_cfg, rar_vrbs);
    ASSERT_TRUE(coreset0_lims.contains(rar_crbs)) << "RAR outside of initial active DL BWP RB limits";

    ASSERT_FALSE(ra_rntis.count(ra_rnti)) << fmt::format("Repeated RA-rnti={} detected", ra_rnti);
    ra_rntis.emplace(ra_rnti);

    for (const auto& ul_grant : rar.grants) {
      ASSERT_FALSE(tc_rntis.count(ul_grant.temp_crnti))
          << fmt::format("Repeated TC-RNTI={} detected", ul_grant.temp_crnti);
      tc_rntis.emplace(ul_grant.temp_crnti);
    }
  }
}

void ocudu::test_pdsch_ue_consistency(const cell_configuration& cell_cfg, span<const dl_msg_alloc> grants)
{
  ASSERT_TRUE(
      test_helper::is_valid_dl_msg_alloc_list(grants, cell_cfg.params.dl_cfg_common.init_dl_bwp.pdcch_common.coreset0));
}

static void
test_pdsch_cross_consistency(const cell_configuration& cell_cfg, slot_point sl_tx, const dl_sched_result& result)
{
  if (not result.csi_rs.empty()) {
    // If CSI-RS is scheduled, it should not collide with common grants.
    ASSERT_TRUE(result.bc.sibs.empty());
    ASSERT_TRUE(result.rar_grants.empty());
    ASSERT_TRUE(result.paging_grants.empty());
  }
}

void ocudu::test_pusch_ue_consistency(const cell_configuration& cell_cfg, span<const ul_sched_info> grants)
{
  ASSERT_LE(grants.size(), cell_cfg.expert_cfg.ue.max_puschs_per_slot);

  for (const ul_sched_info& grant : grants) {
    ASSERT_TRUE(test_helper::is_valid_ul_sched_info(grant));
  }
}

void ocudu::test_pucch_consistency(const cell_configuration& cell_cfg, span<const pucch_info> pucchs)
{
  ASSERT_LE(pucchs.size(), cell_cfg.expert_cfg.ue.max_pucchs_per_slot);

  constexpr unsigned max_f0_or_f1_multiplexing = pucch_constants::f1::NOF_ICS * pucch_constants::f1::NOF_TD_OCC;

  // Note: The grid at index max_f0_or_f1_multiplexing is used to track the union of all F0/F1 grids.
  // [Implementation defined] This assumes that either Format 0 or Format 1 is used, but not both.
  static_vector<cell_slot_resource_grid, max_f0_or_f1_multiplexing + 1> f0_or_f1_grids(
      max_f0_or_f1_multiplexing + 1,
      cell_slot_resource_grid(cell_cfg.params.ul_cfg_common.freq_info_ul.scs_carrier_list));

  // Note: The grid at index max_f4_multiplexing is used to track the union of all F4 grids.
  constexpr unsigned                                              max_f4_multiplexing = 4;
  static_vector<cell_slot_resource_grid, max_f4_multiplexing + 1> f4_grids(
      max_f4_multiplexing + 1, cell_slot_resource_grid(cell_cfg.params.ul_cfg_common.freq_info_ul.scs_carrier_list));

  // For formats that are not multiplexed.
  cell_slot_resource_grid general_grid(cell_cfg.params.ul_cfg_common.freq_info_ul.scs_carrier_list);

  for (const pucch_info& pucch : pucchs) {
    const auto pucch_grants = get_pucch_grant_info(pucch);
    switch (pucch.format()) {
      case pucch_format::FORMAT_0: {
        const auto&    f0_params        = std::get<pucch_format_0>(pucch.format_params);
        const unsigned multiplexing_idx = f0_params.initial_cyclic_shift;
        // Multiplexed by initial cyclic shift only.
        // Check the general grid, the union of the F4 grids and the F0 specific grid.
        // Write to both the F0 union grid and the F0 specific grid.
        ASSERT_FALSE(general_grid.collides(pucch_grants.first));
        ASSERT_FALSE(f4_grids[max_f4_multiplexing].collides(pucch_grants.first));
        ASSERT_FALSE(f0_or_f1_grids[multiplexing_idx].collides(pucch_grants.first));
        f0_or_f1_grids[max_f0_or_f1_multiplexing].fill(pucch_grants.first);
        f0_or_f1_grids[multiplexing_idx].fill(pucch_grants.first);
        if (not pucch.resources.second_hop_prbs.empty()) {
          ASSERT_FALSE(general_grid.collides(*pucch_grants.second));
          ASSERT_FALSE(f4_grids[max_f4_multiplexing].collides(*pucch_grants.second));
          ASSERT_FALSE(f0_or_f1_grids[multiplexing_idx].collides(*pucch_grants.second));
          f0_or_f1_grids[max_f0_or_f1_multiplexing].fill(*pucch_grants.second);
          f0_or_f1_grids[multiplexing_idx].fill(*pucch_grants.second);
        }
      } break;
      case pucch_format::FORMAT_1: {
        const auto&    f1_params = std::get<pucch_format_1>(pucch.format_params);
        const unsigned multiplexing_idx =
            f1_params.initial_cyclic_shift + f1_params.time_domain_occ * pucch_constants::f0::NOF_ICS;
        // Multiplexed by initial cyclic shift and time domain OCC.
        // Check the general grid, the union of the F4 grids and the F1 specific grid.
        // Write to both the F1 union grid and the F1 specific grid.
        ASSERT_FALSE(general_grid.collides(pucch_grants.first));
        ASSERT_FALSE(f4_grids[max_f4_multiplexing].collides(pucch_grants.first));
        ASSERT_FALSE(f0_or_f1_grids[multiplexing_idx].collides(pucch_grants.first));
        f0_or_f1_grids[max_f0_or_f1_multiplexing].fill(pucch_grants.first);
        f0_or_f1_grids[multiplexing_idx].fill(pucch_grants.first);
        if (not pucch.resources.second_hop_prbs.empty()) {
          ASSERT_FALSE(general_grid.collides(*pucch_grants.second));
          ASSERT_FALSE(f4_grids[max_f4_multiplexing].collides(*pucch_grants.second));
          ASSERT_FALSE(f0_or_f1_grids[multiplexing_idx].collides(*pucch_grants.second));
          f0_or_f1_grids[max_f0_or_f1_multiplexing].fill(*pucch_grants.second);
          f0_or_f1_grids[multiplexing_idx].fill(*pucch_grants.second);
        }
      } break;
      case pucch_format::FORMAT_4: {
        const auto&    f4_params        = std::get<pucch_format_4>(pucch.format_params);
        const unsigned multiplexing_idx = f4_params.orthog_seq_idx;
        // Multiplexed by orthogonal sequence index.
        // Check the general grid, the union of the F0/F1 grids and the F4 specific grid.
        // Write to both the F4 union grid and the F4 specific grid.
        ASSERT_FALSE(general_grid.collides(pucch_grants.first));
        ASSERT_FALSE(f0_or_f1_grids[max_f0_or_f1_multiplexing].collides(pucch_grants.first));
        ASSERT_FALSE(f4_grids[multiplexing_idx].collides(pucch_grants.first));
        f4_grids[max_f4_multiplexing].fill(pucch_grants.first);
        f4_grids[multiplexing_idx].fill(pucch_grants.first);
        if (not pucch.resources.second_hop_prbs.empty()) {
          ASSERT_FALSE(general_grid.collides(*pucch_grants.second));
          ASSERT_FALSE(f0_or_f1_grids[max_f0_or_f1_multiplexing].collides(*pucch_grants.second));
          ASSERT_FALSE(f4_grids[multiplexing_idx].collides(*pucch_grants.second));
          f4_grids[max_f4_multiplexing].fill(*pucch_grants.second);
          f4_grids[multiplexing_idx].fill(*pucch_grants.second);
        }
      } break;
      default: {
        // Non multiplexed formats.
        // Check the general grid, and the unions of the multiplexed grids.
        // Only write to the general grid.
        ASSERT_FALSE(general_grid.collides(pucch_grants.first));
        ASSERT_FALSE(f0_or_f1_grids[max_f0_or_f1_multiplexing].collides(pucch_grants.first));
        ASSERT_FALSE(f4_grids[max_f4_multiplexing].collides(pucch_grants.first));
        general_grid.fill(pucch_grants.first);
        if (not pucch.resources.second_hop_prbs.empty()) {
          ASSERT_FALSE(general_grid.collides(*pucch_grants.second));
          ASSERT_FALSE(f0_or_f1_grids[max_f0_or_f1_multiplexing].collides(*pucch_grants.second));
          ASSERT_FALSE(f4_grids[max_f4_multiplexing].collides(*pucch_grants.second));
          general_grid.fill(*pucch_grants.second);
        }
      } break;
    }
  }
}

/// \brief Tests the validity of the parameters chosen for the PDCCHs using common search spaces. Checks include:
/// - PDSCH time resource chosen (k0 and symbols) fall in DL symbols
/// - UCI delay chosen falls in an UL slot.
static void test_pdcch_common_consistency(const cell_configuration&        cell_cfg,
                                          slot_point                       pdcch_slot,
                                          span<const pdcch_dl_information> dl_pdcchs)
{
  if (not cell_cfg.is_tdd()) {
    return;
  }
  const auto& init_dl_bwp = cell_cfg.params.dl_cfg_common.init_dl_bwp;
  for (const pdcch_dl_information& pdcch : dl_pdcchs) {
    span<const pdsch_time_domain_resource_allocation> pdsch_td_list;
    std::optional<unsigned>                           time_res;
    std::optional<unsigned>                           k1;
    switch (pdcch.dci.type) {
      case dci_dl_rnti_config_type::si_f1_0:
        pdsch_td_list =
            get_si_rnti_pdsch_time_domain_list(init_dl_bwp.generic_params.cp, cell_cfg.params.dmrs_typeA_pos);
        time_res = pdcch.dci.si_f1_0.time_resource;
        break;
      case dci_dl_rnti_config_type::ra_f1_0:
        pdsch_td_list = get_ra_rnti_pdsch_time_domain_list(
            init_dl_bwp.pdsch_common, init_dl_bwp.generic_params.cp, cell_cfg.params.dmrs_typeA_pos);
        time_res = pdcch.dci.ra_f1_0.time_resource;
        break;
      case dci_dl_rnti_config_type::tc_rnti_f1_0:
        pdsch_td_list = init_dl_bwp.pdsch_common.pdsch_td_alloc_list;
        time_res      = pdcch.dci.tc_rnti_f1_0.time_resource;
        k1            = pdcch.dci.tc_rnti_f1_0.pdsch_harq_fb_timing_indicator + 1;
        break;
      default:
        break;
    }
    if (not time_res.has_value()) {
      // DCI likely using dedicated config.
      continue;
    }

    // Test PDSCH time resource chosen.
    ASSERT_LT(*time_res, pdsch_td_list.size());
    const pdsch_time_domain_resource_allocation& res        = pdsch_td_list[*time_res];
    const slot_point                             pdsch_slot = pdcch_slot + res.k0;
    const ofdm_symbol_range                      active_dl_symbols =
        get_active_tdd_dl_symbols(*cell_cfg.params.tdd_cfg, pdsch_slot.slot_index(), init_dl_bwp.generic_params.cp);
    ASSERT_TRUE(active_dl_symbols.contains(res.symbols)) << "PDSCH must fall in DL symbols";

    // Test HARQ delay chosen.
    if (k1.has_value()) {
      const slot_point pucch_slot = pdsch_slot + *k1;
      ASSERT_TRUE(has_active_tdd_ul_symbols(*cell_cfg.params.tdd_cfg, pucch_slot.slot_index()))
          << "PUCCH must fall in an UL slot";
    }
  }
}

static void test_ul_pdcch_consistency(const cell_configuration&        cell_cfg,
                                      slot_point                       sl_tx,
                                      span<const pdcch_ul_information> ul_pdcchs)
{
  const auto& pusch_td_list = cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;
  for (const pdcch_ul_information& pdcch : ul_pdcchs) {
    if (pdcch.dci.type == dci_ul_rnti_config_type::tc_rnti_f0_0) {
      const auto& dci_tc = pdcch.dci.tc_rnti_f0_0;
      // Msg3 reTx.
      ASSERT_LT(dci_tc.time_resource, pusch_td_list.size());
    }
  }
}

static void assert_rar_grant_msg3_pusch_consistency(const cell_configuration& cell_cfg,
                                                    const rar_ul_grant&       rar_grant,
                                                    const pusch_information&  msg3_pusch)
{
  ASSERT_EQ(rar_grant.temp_crnti, msg3_pusch.rnti);
  ASSERT_TRUE(msg3_pusch.rbs.is_type1());
  ASSERT_TRUE(msg3_pusch.rbs.any()) << fmt::format("Msg3 with temp-c-rnti={} has no RBs", msg3_pusch.rnti);

  unsigned     N_rb_ul_bwp = cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.crbs.length();
  vrb_interval vrbs        = msg3_pusch.rbs.type1();
  uint8_t      pusch_freq_resource =
      ra_frequency_type1_get_riv(ra_frequency_type1_configuration{N_rb_ul_bwp, vrbs.start(), vrbs.length()});
  ASSERT_EQ(rar_grant.freq_resource_assignment, pusch_freq_resource)
      << fmt::format("Mismatch between RAR grant frequency assignment and corresponding Msg3 PUSCH PRBs");
}

static void assert_rar_grant_msg3_pusch_consistency(const cell_configuration&      cell_cfg,
                                                    const cell_resource_allocator& res_grid)
{
  std::set<rnti_t> tc_rntis;
  const auto&      pusch_td_list = cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common->pusch_td_alloc_list;

  span<const pdcch_dl_information> pdcchs = res_grid[0].result.dl.dl_pdcchs;
  for (const pdcch_dl_information& pdcch : pdcchs) {
    if (pdcch.dci.type != dci_dl_rnti_config_type::ra_f1_0) {
      continue;
    }

    // For a given PDCCH for a RAR, search for the respective RAR PDSCH.
    uint8_t k0 =
        cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common.pdsch_td_alloc_list[pdcch.dci.ra_f1_0.time_resource].k0;
    span<const rar_information> rars   = res_grid[k0].result.dl.rar_grants;
    const auto*                 rar_it = std::find_if(
        rars.begin(), rars.end(), [&pdcch](const auto& rar) { return rar.pdsch_cfg.rnti == pdcch.ctx.rnti; });
    ASSERT_NE(rar_it, rars.end());
    const rar_information& rar = *rar_it;

    ASSERT_EQ(rar.pdsch_cfg.codewords.size(), 1);
    const units::bytes rar_pdu_size{8}; // MAC RAR PDU subheader + length (See TS38.321, 6.1.5 and 6.2.3).
    ASSERT_GE(rar.pdsch_cfg.codewords[0].tb_size_bytes.value(), rar_pdu_size.value() * rar.grants.size());

    // For all RAR grants within the same RAR, check that they are consistent with the respective Msg3 PUSCHs.
    for (const rar_ul_grant& rar_grant : rar.grants) {
      ASSERT_LT(rar_grant.time_resource_assignment, pusch_td_list.size());
      uint8_t k2 = ra_helper::get_msg3_delay(cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.scs,
                                             pusch_td_list[rar_grant.time_resource_assignment].k2);

      span<const ul_sched_info> ul_grants = res_grid[k2].result.ul.puschs;
      const auto* it = std::find_if(ul_grants.begin(), ul_grants.end(), [&rar_grant](const auto& ulgrant) {
        return ulgrant.pusch_cfg.rnti == rar_grant.temp_crnti;
      });
      ASSERT_NE(it, ul_grants.end()) << fmt::format("Msg3 was not found for the scheduled RAR grant with tc-rnti={}",
                                                    rar_grant.temp_crnti);
      assert_rar_grant_msg3_pusch_consistency(cell_cfg, rar_grant, it->pusch_cfg);

      ASSERT_EQ(tc_rntis.count(rar_grant.temp_crnti), 0) << fmt::format("Repeated TC-RNTI detected");
      tc_rntis.emplace(rar_grant.temp_crnti);
    }
  }
}

void ocudu::test_dl_resource_grid_collisions(const cell_configuration& cell_cfg, const dl_sched_result& result)
{
  cell_slot_resource_grid grid(cell_cfg.params.dl_cfg_common.freq_info_dl.scs_carrier_list);

  std::vector<test_grant_info> dl_grants = get_dl_grants(cell_cfg, result);
  for (const test_grant_info& test_grant : dl_grants) {
    ASSERT_FALSE(test_grant.grant.crbs.empty()) << "Resource is empty";
    ASSERT_FALSE(grid.collides(test_grant.grant))
        << fmt::format("Resource collision for grant with rnti={}", test_grant.rnti);
    grid.fill(test_grant.grant);
  }
}

void ocudu::test_prach_opportunity_validity(const cell_configuration&       cell_cfg,
                                            slot_point                      sl_tx,
                                            span<const prach_occasion_info> prachs)
{
  if (prachs.empty()) {
    return;
  }

  const rach_config_common& rach_cfg_common = *cell_cfg.params.ul_cfg_common.init_ul_bwp.rach_cfg_common;
  const prach_configuration prach_cfg       = prach_configuration_get(band_helper::get_freq_range(cell_cfg.band()),
                                                                cell_cfg.is_tdd() ? duplex_mode::TDD : duplex_mode::FDD,
                                                                rach_cfg_common.rach_cfg_generic.prach_config_index);

  // PRACH can only occur on SFNs where n_SFN mod x == y. On all other SFNs no occasion may be allocated.
  const bool valid_sfn =
      std::any_of(prach_cfg.y.begin(), prach_cfg.y.end(), [&](uint8_t y) { return sl_tx.sfn() % prach_cfg.x == y; });
  ASSERT_TRUE(valid_sfn) << fmt::format(
      "PRACH occasions allocated on SFN={} which is not a valid PRACH SFN (x={}, y={})",
      sl_tx.sfn(),
      prach_cfg.x,
      fmt::join(prach_cfg.y, ","));

  const uint16_t expected_nof_cs =
      prach_cyclic_shifts_get(to_ra_subcarrier_spacing(rach_cfg_common.msg1_scs),
                              rach_cfg_common.restricted_set,
                              rach_cfg_common.rach_cfg_generic.zero_correlation_zone_config);

  for (const prach_occasion_info& prach : prachs) {
    // Check if the PRACH matches cell configuration.
    ASSERT_EQ(cell_cfg.params.pci, prach.pci);
    ASSERT_EQ(prach_cfg.format, prach.format);
    ASSERT_EQ(prach_cfg.starting_symbol, prach.start_symbol);
    ASSERT_EQ(prach_cfg.nof_occasions_within_slot, prach.nof_prach_occasions);
    ASSERT_EQ(rach_cfg_common.rach_cfg_generic.msg1_fdm, prach.nof_fd_ra);
    ASSERT_LT(prach.index_fd_ra, prach.nof_fd_ra);
    ASSERT_EQ(expected_nof_cs, prach.nof_cs);
    ASSERT_EQ(rach_cfg_common.total_nof_ra_preambles, prach.nof_preamble_indexes);
    if (prach.start_preamble_index != 255) {
      ASSERT_EQ(0, prach.start_preamble_index);
    }
  }
}

void ocudu::test_ul_resource_grid_collisions(const cell_configuration& cell_cfg, const ul_sched_result& result)
{
  cell_slot_resource_grid      grid(cell_cfg.params.ul_cfg_common.freq_info_ul.scs_carrier_list);
  std::vector<test_grant_info> ul_grants = get_ul_grants(cell_cfg, result);
  std::set<rnti_t>             rntis;

  for (const test_grant_info& test_grant : ul_grants) {
    if (test_grant.type == test_grant_info::PUCCH) {
      // Collisions between PUCCH grants are tested in \c test_pucch_consistency.
      grid.fill(test_grant.grant);
      // RNTIs can be repeated in PUCCH grants.
      rntis.emplace(test_grant.rnti);
    }
  }

  for (const test_grant_info& test_grant : ul_grants) {
    if (test_grant.type == test_grant_info::PUCCH) {
      continue;
    }
    ASSERT_FALSE(grid.collides(test_grant.grant))
        << fmt::format("Resource collision for grant with rnti={}", test_grant.rnti);
    grid.fill(test_grant.grant);

    if (test_grant.type == test_grant_info::UE_UL) {
      ASSERT_TRUE(rntis.count(test_grant.rnti) == 0) << fmt::format("Duplicate RNTI detected: {}", test_grant.rnti);
      // TODO: Handle no multiplexing of UCI into PUSCH.
      rntis.emplace(test_grant.rnti);
    }
  }
}

void ocudu::test_ul_consistency(const cell_configuration& cell_cfg, slot_point sl_tx, const ul_sched_result& result)
{
  // Check that UL grant limits are respected.
  ASSERT_LE(result.pucchs.size() + result.puschs.size(), cell_cfg.expert_cfg.ue.max_ul_grants_per_slot);

  ASSERT_NO_FATAL_FAILURE(test_prach_opportunity_validity(cell_cfg, sl_tx, result.prachs));
  ASSERT_NO_FATAL_FAILURE(test_pusch_ue_consistency(cell_cfg, result.puschs));
  ASSERT_NO_FATAL_FAILURE(test_pucch_consistency(cell_cfg, result.pucchs));
  ASSERT_NO_FATAL_FAILURE(test_ul_resource_grid_collisions(cell_cfg, result));
}

void ocudu::test_dl_consistency(const cell_configuration& cell_cfg, slot_point sl_tx, const dl_sched_result& result)
{
  ASSERT_NO_FATAL_FAILURE(test_pdsch_sib_consistency(cell_cfg, result.bc.sibs));
  ASSERT_NO_FATAL_FAILURE(test_pdsch_rar_consistency(cell_cfg, result.rar_grants));
  ASSERT_NO_FATAL_FAILURE(test_pdsch_ue_consistency(cell_cfg, result.ue_grants));
  ASSERT_NO_FATAL_FAILURE(test_pdsch_cross_consistency(cell_cfg, sl_tx, result));
  ASSERT_NO_FATAL_FAILURE(test_pdcch_common_consistency(cell_cfg, sl_tx, result.dl_pdcchs));
  ASSERT_NO_FATAL_FAILURE(test_ul_pdcch_consistency(cell_cfg, sl_tx, result.ul_pdcchs));
  ASSERT_NO_FATAL_FAILURE(test_dl_resource_grid_collisions(cell_cfg, result));
}

void ocudu::test_scheduler_result_consistency(const cell_configuration& cell_cfg,
                                              slot_point                sl_tx,
                                              const sched_result&       result)
{
  ASSERT_TRUE(result.success);
  ASSERT_NO_FATAL_FAILURE(assert_tdd_pattern_consistency(cell_cfg, sl_tx, result));
  ASSERT_NO_FATAL_FAILURE(test_dl_consistency(cell_cfg, sl_tx, result.dl));
  ASSERT_NO_FATAL_FAILURE(test_ul_consistency(cell_cfg, sl_tx, result.ul));
}

/// \brief Verifies that the cell resource grid PRBs and symbols was filled with the allocated PDSCHs.
void ocudu::assert_dl_resource_grid_filled(const cell_configuration&      cell_cfg,
                                           const cell_resource_allocator& cell_res_grid)
{
  std::vector<test_grant_info> dl_grants = get_dl_grants(cell_cfg, cell_res_grid[0].result.dl);
  for (const test_grant_info& test_grant : dl_grants) {
    if (test_grant.type != ocudu::test_grant_info::DL_PDCCH and test_grant.type != ocudu::test_grant_info::UL_PDCCH) {
      ASSERT_TRUE(cell_res_grid[0].dl_res_grid.all_set(test_grant.grant))
          << fmt::format("The allocation with rnti={}, type={}, crbs={} was not registered in the cell resource grid",
                         test_grant.rnti,
                         fmt::underlying(test_grant.type),
                         test_grant.grant.crbs);
    }
  }
}

/// \brief Verifies that the cell resource grid PRBs and symbols was filled with the allocated PUCCHs.
bool ocudu::assert_ul_resource_grid_filled(const cell_configuration&      cell_cfg,
                                           const cell_resource_allocator& cell_res_grid,
                                           unsigned                       tx_delay,
                                           bool                           expect_grants)
{
  // The function get_ul_grants() returns 2 test_grant_info per pucch_info if intra_slot_freq_hopping is enabled.
  std::vector<test_grant_info> ul_grants = get_ul_grants(cell_cfg, cell_res_grid[tx_delay].result.ul);
  if (expect_grants and ul_grants.empty()) {
    return false;
  }
  for (const test_grant_info& test_grant : ul_grants) {
    if (test_grant.type == ocudu::test_grant_info::UE_UL || test_grant.type == ocudu::test_grant_info::PUCCH) {
      if (not cell_res_grid[tx_delay].ul_res_grid.all_set(test_grant.grant)) {
        return false;
      }
    }
  }
  return true;
}

bool ocudu::test_res_grid_has_re_set(const cell_resource_allocator& cell_res_grid, grant_info grant, unsigned tx_delay)
{
  return cell_res_grid[tx_delay].ul_res_grid.all_set(grant);
}

void ocudu::test_scheduler_result_consistency(const cell_configuration&      cell_cfg,
                                              const cell_resource_allocator& cell_res_grid)
{
  ASSERT_NO_FATAL_FAILURE(test_scheduler_result_consistency(cell_cfg, cell_res_grid[0].slot, cell_res_grid[0].result));
  assert_pdcch_pdsch_common_consistency(cell_cfg, cell_res_grid);
  assert_dl_resource_grid_filled(cell_cfg, cell_res_grid);
  assert_rar_grant_msg3_pusch_consistency(cell_cfg, cell_res_grid);
}

/// Helper to determine slot index associated with a RACH indication.
static unsigned get_ra_slot_index(const cell_configuration& cell_cfg, slot_point rach_slot_rx)
{
  // As per Section 5.1.3, TS 38.321, and from Section 5.3.2, TS 38.211, slot_idx uses as the numerology of
  // reference 15kHz for long PRACH Formats (i.e, slot_idx = subframe index); whereas, for short PRACH formats, it
  // uses the same numerology as the SCS common (i.e, slot_idx = actual slot index within the frame).
  return is_long_preamble(
             prach_configuration_get(
                 band_helper::get_freq_range(cell_cfg.band()),
                 band_helper::get_duplex_mode(cell_cfg.band()),
                 cell_cfg.params.ul_cfg_common.init_ul_bwp.rach_cfg_common->rach_cfg_generic.prach_config_index)
                 .format)
             ? rach_slot_rx.subframe_index()
             : rach_slot_rx.slot_index();
}

/// Helper to determine the RA-RNTI of a given RACH occasion.
static rnti_t
get_ra_rnti(const cell_configuration& cell_cfg, slot_point rach_slot_rx, const rach_indication_message::occasion& occ)
{
  const unsigned slot_idx = get_ra_slot_index(cell_cfg, rach_slot_rx);
  return ra_helper::get_ra_rnti(slot_idx, occ.start_symbol, occ.frequency_index);
}

static slot_interval get_rar_window(const cell_configuration& cell_cfg, slot_point rach_slot_rx)
{
  slot_point rar_win_start;
  for (unsigned i = 1; i != rach_slot_rx.nof_slots_per_frame(); ++i) {
    if (cell_cfg.is_dl_enabled(rach_slot_rx + i)) {
      rar_win_start = rach_slot_rx + i;
      break;
    }
  }
  return {rar_win_start,
          rar_win_start + cell_cfg.params.ul_cfg_common.init_ul_bwp.rach_cfg_common->rach_cfg_generic.ra_resp_window};
}

test_helper::ra_scheduler_tracker::ra_scheduler_tracker(const cell_configuration& cell_cfg_) : cell_cfg(cell_cfg_) {}

void test_helper::ra_scheduler_tracker::on_new_rach_ind(const rach_indication_message& ind)
{
  for (const auto& occ : ind.occasions) {
    const rnti_t        ra_rnti    = get_ra_rnti(cell_cfg, ind.slot_rx, occ);
    const slot_interval rar_window = get_rar_window(cell_cfg, ind.slot_rx);
    for (const auto& preamb : occ.preambles) {
      preamble_context ctxt;
      ctxt.ra_rnti  = ra_rnti;
      ctxt.preamble = preamb;
      ctxt.rar_win  = rar_window;
      pending_preambles.push_back(ctxt);
    }
  }
}

void test_helper::ra_scheduler_tracker::on_crc_indication(const ul_crc_indication& crc)
{
  for (const auto& pdu : crc.crcs) {
    auto it = std::find_if(pending_preambles.begin(), pending_preambles.end(), [&pdu](const preamble_context& preamb) {
      return preamb.preamble.tc_rnti == pdu.rnti;
    });
    if (it != pending_preambles.end()) {
      // Msg3 is being ACKed/NACKed.
      ASSERT_EQ(it->last_msg3_slot, crc.sl_rx)
          << fmt::format("CRC before Msg3 slot? ({} != {})", it->last_msg3_slot, crc.sl_rx);
      if (not it->acked and pdu.tb_crc_success) {
        msg3_ack_counter++;
        it->acked = true;
      }
    }
  }
}

void test_helper::ra_scheduler_tracker::on_new_result(slot_point sl_tx, const sched_result& result)
{
  for (auto& dl_pdcch : result.dl.dl_pdcchs) {
    if (dl_pdcch.dci.type != dci_dl_rnti_config_type::ra_f1_0) {
      // Skip PDCCHs that are not for RAR.
      continue;
    }
    auto it = std::find_if(pending_preambles.begin(),
                           pending_preambles.end(),
                           [&dl_pdcch](const preamble_context& preamb) { return preamb.ra_rnti == dl_pdcch.ctx.rnti; });
    ASSERT_NE(it, pending_preambles.end()) << "RA-RNTI scheduled with no associated RACH preamble";
    ++ra_dl_pdcch_ack_counter;

    const auto& td_res =
        get_ra_rnti_pdsch_time_domain_list(cell_cfg.params.dl_cfg_common.init_dl_bwp.pdsch_common,
                                           cell_cfg.params.dl_cfg_common.init_dl_bwp.generic_params.cp,
                                           cell_cfg.params.dmrs_typeA_pos)[dl_pdcch.dci.ra_f1_0.time_resource];
    auto& expected_rar      = pending_rars.emplace_back();
    expected_rar.ra_rnti    = dl_pdcch.ctx.rnti;
    expected_rar.pdcch_slot = sl_tx;
    expected_rar.rar_slot   = sl_tx + td_res.k0;
    expected_rar.symbols    = td_res.symbols;
  }

  for (auto& rar : result.dl.rar_grants) {
    // Check if the DL PDCCH matches the RAR content.
    auto expected_rar_it = std::find_if(pending_rars.begin(), pending_rars.end(), [&sl_tx, &rar](const auto& pending) {
      return pending.rar_slot == sl_tx and pending.ra_rnti == rar.pdsch_cfg.rnti;
    });
    ASSERT_NE(expected_rar_it, pending_rars.end()) << "RAR scheduled with no associated PDCCH";
    ASSERT_EQ(expected_rar_it->symbols, rar.pdsch_cfg.symbols);
    expected_rar_it->scheduled = true;
    ++rar_counter;

    for (auto& ul_grant : rar.grants) {
      // Find RAR UL grant with matching and RA-RNTI and TC-RNTI.
      auto it = std::find_if(
          pending_preambles.begin(), pending_preambles.end(), [&rar, &ul_grant](const preamble_context& preamb) {
            return preamb.ra_rnti == rar.pdsch_cfg.rnti and preamb.preamble.tc_rnti == ul_grant.temp_crnti;
          });
      ASSERT_NE(it, pending_preambles.end()) << "RAR scheduled with an RAR UL grant with no associated RACH preamble";

      // RAR UL grant is scheduled for newTx.
      auto& ctxt = *it;
      ASSERT_FALSE(ctxt.rar_slot.valid()) << "Duplicate RAR per RACH preamble";
      ctxt.rar_slot = sl_tx;
      ASSERT_TRUE(ctxt.rar_win.contains(sl_tx)) << "RAR outside of RAR window";
      ASSERT_EQ(ctxt.preamble.preamble_id, ul_grant.rapid);
      auto td_list = get_pusch_time_domain_resource_table(*cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common);
      ctxt.first_msg3_slot = ctxt.rar_slot + ra_helper::get_msg3_delay(cell_cfg.scs_common(),
                                                                       td_list[ul_grant.time_resource_assignment].k2);
    }
  }

  // Handle Msg3 reTxs.
  const auto& pusch_td_list =
      get_pusch_time_domain_resource_table(*cell_cfg.params.ul_cfg_common.init_ul_bwp.pusch_cfg_common);
  for (auto& ul_pdcch : result.dl.ul_pdcchs) {
    if (ul_pdcch.dci.type != dci_ul_rnti_config_type::tc_rnti_f0_0) {
      continue;
    }

    auto preamble_it =
        std::find_if(pending_preambles.begin(), pending_preambles.end(), [&ul_pdcch](const preamble_context& preamb) {
          return preamb.preamble.tc_rnti == ul_pdcch.ctx.rnti;
        });
    ASSERT_NE(preamble_it, pending_preambles.end()) << "Msg3 reTx UL PDCCH has no associated RACH preamble";
    ASSERT_GT(sl_tx, preamble_it->first_msg3_slot) << "Msg3 reTx UL PDCCH cannot be before Msg3 newTx";

    const auto& dci    = ul_pdcch.dci.tc_rnti_f0_0;
    const auto& td_res = pusch_td_list[dci.time_resource];

    const slot_point pusch_slot = sl_tx + td_res.k2;
    auto             pending_msg3_it =
        std::find_if(pending_msg3_retxs.begin(),
                     pending_msg3_retxs.end(),
                     [&ul_pdcch, pusch_slot](const msg3_retx_context& pending_msg3) {
                       return pending_msg3.tc_rnti == ul_pdcch.ctx.rnti and pending_msg3.pusch_slot == pusch_slot;
                     });
    ASSERT_EQ(pending_msg3_it, pending_msg3_retxs.end()) << "Duplicate Msg3 reTx UL PDCCH";

    auto& pending_msg3         = pending_msg3_retxs.emplace_back();
    pending_msg3.tc_rnti       = ul_pdcch.ctx.rnti;
    pending_msg3.pdcch_slot    = sl_tx;
    pending_msg3.pusch_slot    = pusch_slot;
    pending_msg3.ss_id         = ul_pdcch.ctx.context.ss_id;
    pending_msg3.symbols       = td_res.symbols;
    pending_msg3.k2            = td_res.k2;
    pending_msg3.freq_resource = dci.frequency_resource;
    pending_msg3.mcs           = dci.modulation_coding_scheme;
    pending_msg3.rv            = dci.redundancy_version;
  }

  for (auto& pusch : result.ul.puschs) {
    const bool is_msg3 = pusch.context.ue_index == INVALID_DU_UE_INDEX;
    if (not is_msg3) {
      continue;
    }

    // Search respective RACH preamble.
    auto it =
        std::find_if(pending_preambles.begin(), pending_preambles.end(), [&pusch](const preamble_context& preamb) {
          return pusch.pusch_cfg.rnti == preamb.preamble.tc_rnti;
        });
    ASSERT_NE(it, pending_preambles.end()) << "RAR UL grant has no associated RACH preamble";
    auto& ctxt = *it;
    ASSERT_TRUE(ctxt.rar_slot.valid()) << "RAR UL grant before RAR";
    ctxt.last_msg3_slot = sl_tx;

    if (pusch.context.nof_retxs == 0) {
      // It is the first Msg3 tx.
      ++msg3_newtx_counter;
      ASSERT_EQ(ctxt.first_msg3_slot, sl_tx);
      ASSERT_TRUE(pusch.context.msg3_delay.has_value());
      ASSERT_EQ(ctxt.first_msg3_slot - ctxt.rar_slot, *pusch.context.msg3_delay);
    } else {
      ++msg3_retx_counter;
      ASSERT_GT(sl_tx, ctxt.first_msg3_slot);
      ASSERT_FALSE(pusch.pusch_cfg.new_data);

      // PDCCH and Msg3 PUSCH match in content.
      auto expected_msg3_it = std::find_if(
          pending_msg3_retxs.begin(), pending_msg3_retxs.end(), [&pusch, sl_tx](const msg3_retx_context& pending_msg3) {
            return pending_msg3.tc_rnti == pusch.pusch_cfg.rnti and pending_msg3.pusch_slot == sl_tx;
          });
      ASSERT_NE(expected_msg3_it, pending_msg3_retxs.end()) << "Msg3 reTx PUSCH has no associated UL PDCCH";
      ASSERT_EQ(expected_msg3_it->ss_id, pusch.context.ss_id);
      ASSERT_EQ(expected_msg3_it->k2, pusch.context.k2);
      ASSERT_EQ(expected_msg3_it->symbols, pusch.pusch_cfg.symbols);

      const unsigned N_rb_ul_bwp = cell_cfg.params.ul_cfg_common.init_ul_bwp.generic_params.crbs.length();
      const auto     vrbs        = pusch.pusch_cfg.rbs.type1();
      const uint8_t  freq_res =
          ra_frequency_type1_get_riv(ra_frequency_type1_configuration{N_rb_ul_bwp, vrbs.start(), vrbs.length()});
      ASSERT_EQ(expected_msg3_it->freq_resource, freq_res)
          << "Mismatch between Msg3 reTx UL PDCCH frequency assignment and corresponding PUSCH PRBs";
      ASSERT_EQ(expected_msg3_it->mcs, pusch.pusch_cfg.mcs_index.value());
      ASSERT_EQ(expected_msg3_it->rv, pusch.pusch_cfg.rv_index);

      pending_msg3_retxs.erase(expected_msg3_it);
    }
  }

  // Pop expired expected RARs.
  while (not pending_rars.empty() and pending_rars.front().rar_slot >= sl_tx) {
    ASSERT_TRUE(pending_rars.front().scheduled) << "DL PDCCH was scheduled but RAR was not";
    pending_rars.pop_front();
  }

  for (auto it = pending_msg3_retxs.begin(); it != pending_msg3_retxs.end(); ++it) {
    ASSERT_GT(it->pusch_slot, sl_tx) << fmt::format(
        "UL PDCCH in slot {} did not lead to a Msg3 reTx PUSCH in slot {} for tc-rnti={}",
        it->pdcch_slot,
        it->pusch_slot,
        it->tc_rnti);
  }

  // Pop expired RACH preambles.
  while (not pending_preambles.empty() and is_expired(pending_preambles.front(), sl_tx)) {
    pending_preambles.pop_front();
  }
}

bool test_helper::ra_scheduler_tracker::is_expired(const preamble_context& ctxt, slot_point sl_tx) const
{
  if (ctxt.acked) {
    return true;
  }
  if (not ctxt.rar_slot.valid() and ctxt.rar_win.stop() < sl_tx) {
    // RAR window expired.
    return true;
  }
  static constexpr unsigned max_slots_for_msg3_ack = 256;
  if (ctxt.last_msg3_slot.valid() and ctxt.last_msg3_slot + max_slots_for_msg3_ack < sl_tx) {
    // Msg3 was not ACKed.
    return true;
  }
  return false;
}
