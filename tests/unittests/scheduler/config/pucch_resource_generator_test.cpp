// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/scheduler/support/pucch/pucch_collision_info.h"
#include "ocudu/ran/bwp/bwp_configuration.h"
#include "ocudu/ran/pucch/pucch_configuration.h"
#include "ocudu/ran/pucch/pucch_constants.h"
#include "ocudu/ran/pucch/pucch_mapping.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/scheduler/config/pucch_resource_builder_params.h"
#include "ocudu/scheduler/config/pucch_resource_generator.h"
#include "ocudu/scheduler/config/serving_cell_config.h"
#include "ocudu/scheduler/config/serving_cell_config_factory.h"
#include "ocudu/scheduler/config/ue_bwp_config.h"
#include <gtest/gtest.h>
#include <limits>

using namespace ocudu;

struct pucch_res_gen_test_params {
  pucch_resource_builder_params builder_params;
  bwp_configuration             bwp_cfg;
};

class pucch_resource_generator_test : public ::testing::TestWithParam<pucch_res_gen_test_params>
{
public:
  pucch_resource_generator_test() : params(GetParam().builder_params), bwp_cfg(GetParam().bwp_cfg) {}

protected:
  const pucch_resource_builder_params params;
  const bwp_configuration             bwp_cfg;
};

TEST_P(pucch_resource_generator_test, successful_generation_results_in_no_collisions)
{
  std::vector<pucch_resource> res_list = config_helpers::generate_cell_pucch_res_list(params, bwp_cfg.crbs.length());
  ASSERT_FALSE(res_list.empty());

  const unsigned nof_res_f0_f1 =
      params.nof_cell_sr_resources + params.nof_cell_res_set_configs * params.res_set_0_size.value();
  const unsigned nof_res_f2_f3_f4 =
      params.nof_cell_csi_resources + params.nof_cell_res_set_configs * params.res_set_1_size.value();
  ASSERT_EQ(nof_res_f0_f1 + nof_res_f2_f3_f4, res_list.size());

  // Generated resources should not collide with each other.
  // TODO: handle F0+F2 case.
  std::vector<pucch_collision_info> collision_infos;
  collision_infos.reserve(res_list.size());
  for (const auto& res : res_list) {
    collision_infos.emplace_back(res, bwp_cfg);
  }
  for (unsigned i = 0; i != collision_infos.size(); ++i) {
    for (unsigned j = i + 1; j != collision_infos.size(); ++j) {
      ASSERT_FALSE(collision_infos[i].collides(collision_infos[j]));
    }
  }
}

TEST_P(pucch_resource_generator_test, ue_pucch_config_builder_test)
{
  const auto cell_res_list = config_helpers::generate_cell_pucch_res_list(params, bwp_cfg.crbs.length());

  for (auto res_set_cfg_id     = pucch_resource_set_config_id(0),
            res_set_cfg_id_end = pucch_resource_set_config_id(params.nof_cell_res_set_configs);
       res_set_cfg_id != res_set_cfg_id_end;
       ++res_set_cfg_id) {
    for (auto sr_res_id = pucch_sr_resource_id(0), sr_res_id_end = pucch_sr_resource_id(params.nof_cell_sr_resources);
         sr_res_id != sr_res_id_end;
         ++sr_res_id) {
      for (auto csi_res_id     = pucch_csi_resource_id(0),
                csi_res_id_end = pucch_csi_resource_id(params.nof_cell_csi_resources);
           csi_res_id != csi_res_id_end;
           ++csi_res_id) {
        const ue_pucch_config        ue_pucch_cfg{.res_set_cfg_id = res_set_cfg_id, .sr_res_id = sr_res_id};
        const ue_periodic_csi_config ue_csi_cfg{.pucch_res_id = csi_res_id};
        serving_cell_config          serv_cell_cfg = config_helpers::create_default_initial_ue_serving_cell_config();

        // Verify that the builder succeeds for all valid configuration ID combinations.
        ASSERT_TRUE(
            config_helpers::ue_pucch_config_builder(serv_cell_cfg, cell_res_list, params, ue_pucch_cfg, ue_csi_cfg));

        const auto&               pucch_cfg          = serv_cell_cfg.ul_config->init_ul_bwp.pucch_cfg.value();
        static constexpr unsigned nof_sr_res_per_ue  = 1;
        static constexpr unsigned nof_csi_res_per_ue = 1;
        // For F0+F2 case:
        // - Two extra resources are added to the UE resource list (CSI_F0 and SR_F2).
        // - Two extra resources are added to Resource Set ID 0 (CSI_F0 and SR_F0).
        // - Two extra resources are added to Resource Set ID 1 (CSI_F2 and SR_F2).
        const unsigned extra_res_in_res_list =
            params.format_01() == pucch_format::FORMAT_0 and params.format_234() == pucch_format::FORMAT_2 ? 2 : 0;
        const unsigned extra_res_per_res_set =
            params.format_01() == pucch_format::FORMAT_0 and params.format_234() == pucch_format::FORMAT_2 ? 2 : 0;

        ASSERT_EQ(params.res_set_0_size.value() + nof_sr_res_per_ue + params.res_set_1_size.value() +
                      nof_csi_res_per_ue + extra_res_in_res_list,
                  pucch_cfg.pucch_res_list.size());

        // Check the SR resource.
        const pucch_res_id_t expected_sr_res_id{params.get_sr_cell_res_idx(sr_res_id), params.get_sr_ue_res_idx()};

        ASSERT_EQ(1U, pucch_cfg.sr_res_list.size());
        ASSERT_EQ(pucch_cfg.sr_res_list.front().pucch_res_id, expected_sr_res_id);

        const pucch_resource& sr_res = pucch_cfg.pucch_res_list[expected_sr_res_id.ue_res_id];
        ASSERT_EQ(params.format_01(), sr_res.format);
        ASSERT_EQ(expected_sr_res_id, sr_res.res_id);

        // Check the CSI resource.
        const pucch_res_id_t expected_csi_res_id{params.get_csi_cell_res_idx(csi_res_id), params.get_csi_ue_res_idx()};

        ASSERT_EQ(1U, serv_cell_cfg.csi_meas_cfg->csi_report_cfg_list.size());
        const auto& periodic_report_cfg = std::get<csi_report_config::periodic_or_semi_persistent_report_on_pucch>(
            serv_cell_cfg.csi_meas_cfg->csi_report_cfg_list.front().report_cfg_type);
        ASSERT_EQ(1U, periodic_report_cfg.pucch_csi_res_list.size());
        ASSERT_EQ(expected_csi_res_id, periodic_report_cfg.pucch_csi_res_list.front().pucch_res_id);

        const pucch_resource& csi_res = pucch_cfg.pucch_res_list[expected_csi_res_id.ue_res_id];
        ASSERT_EQ(params.format_234(), csi_res.format);
        ASSERT_EQ(expected_csi_res_id, csi_res.res_id);

        // Check Resource Set ID 0.
        const auto& res_set_0 = pucch_cfg.pucch_res_set[0];
        ASSERT_EQ(pucch_res_set_idx::set_0, res_set_0.pucch_res_set_id);
        ASSERT_EQ(params.res_set_0_size.value() + extra_res_per_res_set, res_set_0.pucch_res_id_list.size());

        for (unsigned pri = 0; pri != params.res_set_0_size.value(); ++pri) {
          const pucch_res_id_t expected_res_id{params.get_res_set_0_cell_res_idx(res_set_cfg_id, pri),
                                               params.get_res_set_0_ue_res_idx(pri)};

          ASSERT_EQ(expected_res_id, res_set_0.pucch_res_id_list[pri]);

          const pucch_resource& res = pucch_cfg.pucch_res_list[expected_res_id.ue_res_id];
          ASSERT_EQ(params.format_01(), res.format);
          ASSERT_EQ(expected_res_id, res.res_id);
        }

        if (extra_res_per_res_set) {
          // Check CSI_F0 and SR_F0.
          const pucch_res_id_t expected_id_csi_f0{std::numeric_limits<unsigned>::max(), params.res_set_0_size.value()};
          ASSERT_EQ(expected_id_csi_f0, res_set_0.pucch_res_id_list[params.res_set_0_size.value()]);
          ASSERT_EQ(expected_sr_res_id, res_set_0.pucch_res_id_list[params.res_set_0_size.value() + 1]);
        }

        // Check Resource Set ID 1.
        const auto& res_set_1 = pucch_cfg.pucch_res_set[1];
        ASSERT_EQ(pucch_res_set_idx::set_1, res_set_1.pucch_res_set_id);
        ASSERT_EQ(params.res_set_0_size.value() + extra_res_per_res_set, res_set_1.pucch_res_id_list.size());

        for (unsigned pri = 0; pri != params.res_set_1_size.value(); ++pri) {
          const pucch_res_id_t expected_res_id{params.get_res_set_1_cell_res_idx(res_set_cfg_id, pri),
                                               params.get_res_set_1_ue_res_idx(pri)};
          ASSERT_EQ(expected_res_id, res_set_1.pucch_res_id_list[pri]);

          const pucch_resource& res = pucch_cfg.pucch_res_list[expected_res_id.ue_res_id];
          ASSERT_EQ(params.format_234(), res.format);
          ASSERT_EQ(expected_res_id, res.res_id);
        }

        if (extra_res_per_res_set) {
          // Check CSI_F2 and SR_F2.
          ASSERT_EQ(expected_csi_res_id, res_set_1.pucch_res_id_list[params.res_set_0_size.value()]);
          const pucch_res_id_t expected_id_sr_f2{std::numeric_limits<unsigned>::max(),
                                                 static_cast<unsigned>(pucch_cfg.pucch_res_list.size()) - 1};
          ASSERT_EQ(expected_id_sr_f2, res_set_1.pucch_res_id_list[params.res_set_0_size.value() + 1]);
        }

        // Check that all UE resources are equal to their corresponding cell resources.
        for (const auto& ue_res : pucch_cfg.pucch_res_list) {
          if (ue_res.res_id.cell_res_id == std::numeric_limits<unsigned>::max()) {
            // Skip dummy resources (CSI_F0 and SR_F2).
            continue;
          }
          const auto& cell_res = cell_res_list[ue_res.res_id.cell_res_id];

          // Everything must be equal, except for the resource ID.
          ASSERT_EQ(cell_res.starting_prb, ue_res.starting_prb);
          ASSERT_EQ(cell_res.second_hop_prb, ue_res.second_hop_prb);
          ASSERT_EQ(cell_res.nof_symbols, ue_res.nof_symbols);
          ASSERT_EQ(cell_res.starting_sym_idx, ue_res.starting_sym_idx);
          ASSERT_EQ(cell_res.format, ue_res.format);
          ASSERT_EQ(cell_res.format_params, ue_res.format_params);
        }
      }
    }
  }
}

// Standard BWP configuration: 15 kHz SCS, 5 MHz BW.
static constexpr bwp_configuration default_bwp_cfg{.cp   = cyclic_prefix::NORMAL,
                                                   .scs  = subcarrier_spacing::kHz15,
                                                   .crbs = crb_interval::start_and_len(0, 25)};

static constexpr pucch_f0_params f0_max_density{
    .nof_syms = 1,
};
static constexpr pucch_f0_params f0_freq_hop{
    .nof_syms               = 2,
    .intraslot_freq_hopping = true,
};

static constexpr pucch_f1_params f1_low_density{
    .nof_syms       = pucch_constants::f1::NOF_SYMS.stop(),
    .nof_cyc_shifts = pucch_nof_cyclic_shifts::no_cyclic_shift,
    .occ_supported  = false,

};
static constexpr pucch_f1_params f1_high_density{
    .nof_syms       = pucch_constants::f1::NOF_SYMS.start(),
    .nof_cyc_shifts = pucch_nof_cyclic_shifts::twelve,
    .occ_supported  = true,
};

static constexpr pucch_f2_params f2_multiple_rbs{
    .max_nof_rbs = 3,
};
static constexpr pucch_f2_params f2_freq_hop{
    .nof_syms               = 2,
    .intraslot_freq_hopping = true,
};

static constexpr pucch_f3_params f3_low_density{.nof_syms = 7, .max_nof_rbs = 1, .intraslot_freq_hopping = true};
static constexpr pucch_f3_params f3_high_density{.nof_syms = pucch_constants::f3::NOF_SYMS.start()};

static constexpr pucch_f4_params f4_low_density{.nof_syms = 7, .intraslot_freq_hopping = true};
static constexpr pucch_f4_params f4_high_density{.nof_syms      = pucch_constants::f3::NOF_SYMS.start(),
                                                 .occ_supported = true,
                                                 .occ_length    = pucch_f4_occ_len::n4};

// The test cases are designed with the following considerations:
// - Test different combinations of PUCCH Formats and parameters.
// - The number of resources are sized s.t. the PUCCH cover as much of the BWP as possible (<= 50%), so that we can:
//   - Increase the chance of detecting regressions in the resource generation logic.
//   - See the effect of the format parameters on the number of resources (and consequenty on the UE cell capacity).
INSTANTIATE_TEST_SUITE_P(,
                         pucch_resource_generator_test,
                         ::testing::Values(
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 1,
                                         .nof_cell_sr_resources    = 3,
                                         .nof_cell_csi_resources   = 1,
                                         .f0_or_f1_params          = f1_low_density,
                                         .f2_or_f3_or_f4_params    = f2_multiple_rbs,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 10,
                                         .nof_cell_sr_resources    = 84,
                                         .nof_cell_csi_resources   = 10,
                                         .f0_or_f1_params          = f1_high_density,
                                         .f2_or_f3_or_f4_params    = f2_freq_hop,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 2,
                                         .nof_cell_sr_resources    = 2,
                                         .nof_cell_csi_resources   = 9,
                                         .f0_or_f1_params          = f0_freq_hop,
                                         .f2_or_f3_or_f4_params    = f2_multiple_rbs,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 8,
                                         .nof_cell_sr_resources    = 8,
                                         .nof_cell_csi_resources   = 8,
                                         .f0_or_f1_params          = f0_max_density,
                                         .f2_or_f3_or_f4_params    = f2_freq_hop,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 1,
                                         .nof_cell_sr_resources    = 2,
                                         .nof_cell_csi_resources   = 2,
                                         .f0_or_f1_params          = f1_low_density,
                                         .f2_or_f3_or_f4_params    = f3_low_density,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 5,
                                         .nof_cell_sr_resources    = 42,
                                         .nof_cell_csi_resources   = 3,
                                         .f0_or_f1_params          = f1_high_density,
                                         .f2_or_f3_or_f4_params    = f3_high_density,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 1,
                                         .nof_cell_sr_resources    = 2,
                                         .nof_cell_csi_resources   = 2,
                                         .f0_or_f1_params          = f1_low_density,
                                         .f2_or_f3_or_f4_params    = f4_low_density,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             },
                             pucch_res_gen_test_params{
                                 .builder_params =
                                     pucch_resource_builder_params{
                                         .nof_cell_res_set_configs = 20,
                                         .nof_cell_sr_resources    = 8,
                                         .nof_cell_csi_resources   = 8,
                                         .f0_or_f1_params          = f1_high_density,
                                         .f2_or_f3_or_f4_params    = f4_high_density,
                                     },
                                 .bwp_cfg = default_bwp_cfg,
                             }));
