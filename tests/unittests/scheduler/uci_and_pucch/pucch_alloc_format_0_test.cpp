/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "../test_utils/scheduler_test_suite.h"
#include "pucch_alloc_base_tester.h"
#include "uci_test_utils.h"
#include <gtest/gtest.h>

using namespace ocudu;

///////   Test PUCCH Format 0.    ///////

class pucch_alloc_format_0_test : public ::testing::Test, public pucch_allocator_base_test
{
public:
  pucch_alloc_format_0_test() :
    pucch_allocator_base_test(
        test_bench_params{.pucch_ded_params = {.f0_or_f1_params = pucch_f0_params{}}, .pucch_res_common = 0U}),
    pucch_res_idx_f0_for_sr(t_bench.get_main_ue()
                                .get_pcell()
                                .cfg()
                                .init_bwp()
                                .ul_ded->pucch_cfg.value()
                                .pucch_res_set[pucch_res_set_idx_to_uint(pucch_res_set_idx::set_0)]
                                .pucch_res_id_list.size() -
                            1U),
    pucch_res_idx_f0_for_csi(t_bench.get_main_ue()
                                 .get_pcell()
                                 .cfg()
                                 .init_bwp()
                                 .ul_ded->pucch_cfg.value()
                                 .pucch_res_set[pucch_res_set_idx_to_uint(pucch_res_set_idx::set_0)]
                                 .pucch_res_id_list.size() -
                             2U),
    pucch_res_idx_f2_for_sr(t_bench.get_main_ue()
                                .get_pcell()
                                .cfg()
                                .init_bwp()
                                .ul_ded->pucch_cfg.value()
                                .pucch_res_set[pucch_res_set_idx_to_uint(pucch_res_set_idx::set_1)]
                                .pucch_res_id_list.size() -
                            1U),
    pucch_res_idx_f2_for_csi(t_bench.get_main_ue()
                                 .get_pcell()
                                 .cfg()
                                 .init_bwp()
                                 .ul_ded->pucch_cfg.value()
                                 .pucch_res_set[pucch_res_set_idx_to_uint(pucch_res_set_idx::set_1)]
                                 .pucch_res_id_list.size() -
                             2U)
  {
    static constexpr max_pucch_code_rate max_code_rate = max_pucch_code_rate::dot_25;

    // Set the expected SR grant to the SR resource.
    pucch_expected_sr = test_helpers::make_ded_pucch_info(
        t_bench.cell_cfg, t_bench.cell_cfg.ded_pucch_resources[6], {.sr_bits = sr_nof_bits::one}, max_code_rate);

    // Set the expected HARQ F1 grant to the first resource in Resource Set ID 0.
    pucch_expected_res_set_0 = test_helpers::make_ded_pucch_info(
        t_bench.cell_cfg, t_bench.cell_cfg.ded_pucch_resources[0], {.harq_ack_nof_bits = 1U}, max_code_rate);

    // Set the expected Resource Set ID 1 HARQ grant to the first resource in Resource Set ID 1.
    pucch_expected_res_set_1 = test_helpers::make_ded_pucch_info(
        t_bench.cell_cfg, t_bench.cell_cfg.ded_pucch_resources[8], {.harq_ack_nof_bits = 3U}, max_code_rate);

    // This PUCCH resource is located on the same symbols and PRBs as the PUCCH Format 0 resource for SR.
    // Doesn't exist in the list of cell resources.
    pucch_res_id_t sr_f2_res_id =
        t_bench.get_main_ue()
            .get_pcell()
            .cfg()
            .init_bwp()
            .ul_ded->pucch_cfg->pucch_res_set[pucch_res_set_idx_to_uint(pucch_res_set_idx::set_1)]
            .pucch_res_id_list[pucch_res_idx_f2_for_sr];
    pucch_expected_sr_f2 = test_helpers::make_ded_pucch_info(
        t_bench.cell_cfg,
        t_bench.get_main_ue().get_pcell().cfg().init_bwp().ul_ded->pucch_cfg->pucch_res_list[sr_f2_res_id.ue_res_id],
        {.harq_ack_nof_bits = 3U},
        max_code_rate);

    // Set the expected HARQ CSI grant to the CSI resource.
    pucch_expected_csi = test_helpers::make_ded_pucch_info(t_bench.cell_cfg,
                                                           t_bench.cell_cfg.ded_pucch_resources[14],
                                                           {.csi_part1_nof_bits = default_csi_part1_bits},
                                                           max_code_rate);
  }

protected:
  // Parameters that are passed by the routine to run the tests.
  pucch_info     pucch_expected_sr;
  pucch_info     pucch_expected_res_set_0;
  pucch_info     pucch_expected_sr_f2;
  pucch_info     pucch_expected_res_set_1;
  pucch_info     pucch_expected_csi;
  const unsigned pucch_res_idx_f0_for_sr;
  const unsigned pucch_res_idx_f0_for_csi;
  const unsigned pucch_res_idx_f2_for_sr;
  const unsigned pucch_res_idx_f2_for_csi;

  static constexpr unsigned default_csi_part1_bits = 4U;
};

TEST_F(pucch_alloc_format_0_test, test_sr_allocation_only)
{
  alloc_sr_opportunity(t_bench.get_main_ue());

  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_sr](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_only)
{
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(0U, pri);

  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(
      find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_res_set_0](const auto& pdu) {
        return pucch_info_match(expected, pdu);
      }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_2_bits)
{
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(0U, pri);

  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());

  auto pri_new = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri_new.has_value());
  ASSERT_EQ(pri, pri_new);

  // PUCCH resource indicator after the second allocation should not have changed.
  pucch_expected_res_set_0.uci_bits.harq_ack_nof_bits = 2U;
  ASSERT_EQ(1, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(
      find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_res_set_0](const auto& pdu) {
        return pucch_info_match(expected, pdu);
      }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_over_sr)
{
  alloc_sr_opportunity(t_bench.get_main_ue());

  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f0_for_sr, pri);

  // According to the multiplexing procedure defined by TS 38.213, Section 9.2.5, the resource to use to report 1
  // HARQ-ACK bit + 1 SR bit is the HARQ-ACK resource. However, to circumvent the lack of capability of some UES (that
  // cannot transmit more than 1 PUCCH), we set last resource of PUCCH resource set 0 to be the SR resource and the UE
  // will use this.
  pucch_expected_sr.uci_bits.harq_ack_nof_bits = 1U;
  pucch_expected_sr.uci_bits.sr_bits           = sr_nof_bits::one;
  ASSERT_EQ(1, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_sr](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_2_bits_over_sr)
{
  alloc_sr_opportunity(t_bench.get_main_ue());

  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f0_for_sr, pri);

  pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f0_for_sr, pri);

  pucch_expected_sr.uci_bits.harq_ack_nof_bits = 2U;
  pucch_expected_sr.uci_bits.sr_bits           = sr_nof_bits::one;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_sr](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_3_bits_over_sr)
{
  alloc_sr_opportunity(t_bench.get_main_ue());

  alloc_ded_harq_ack(t_bench.get_main_ue());
  alloc_ded_harq_ack(t_bench.get_main_ue());
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f2_for_sr, pri);

  pucch_expected_sr_f2.uci_bits.sr_bits = sr_nof_bits::one;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_sr_f2](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_4_bits_over_sr)
{
  alloc_sr_opportunity(t_bench.get_main_ue());

  alloc_ded_harq_ack(t_bench.get_main_ue());
  alloc_ded_harq_ack(t_bench.get_main_ue());
  alloc_ded_harq_ack(t_bench.get_main_ue());
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f2_for_sr, pri);

  pucch_expected_sr_f2.uci_bits.harq_ack_nof_bits  = 4U;
  pucch_expected_sr_f2.uci_bits.sr_bits            = sr_nof_bits::one;
  pucch_expected_sr_f2.uci_bits.csi_part1_nof_bits = 0U;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_sr_f2](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_over_csi)
{
  alloc_csi_opportunity(t_bench.get_main_ue(), default_csi_part1_bits);

  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  // The allocation should preserve the pucch_res_idx_f0_for_csi
  ASSERT_EQ(pucch_res_idx_f2_for_csi, pri);

  // After the multiplexing, the PUCCH F2 resource is that one that have the same PUCCH resource indicator as
  // pucch_res_idx_f0_for_csi; we need to update the PRBs and symbols accordingly. With the given configuration, this
  // resource will have the same PRBs and symbols as the F2 resource for SR.
  pucch_expected_csi.uci_bits.harq_ack_nof_bits  = 1U;
  pucch_expected_csi.uci_bits.sr_bits            = sr_nof_bits::no_sr;
  pucch_expected_csi.uci_bits.csi_part1_nof_bits = 4U;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_csi](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_2_bits_over_csi)
{
  alloc_csi_opportunity(t_bench.get_main_ue(), default_csi_part1_bits);

  alloc_ded_harq_ack(t_bench.get_main_ue());
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f2_for_csi, pri);

  pucch_expected_csi.uci_bits.harq_ack_nof_bits  = 2U;
  pucch_expected_csi.uci_bits.sr_bits            = sr_nof_bits::no_sr;
  pucch_expected_csi.uci_bits.csi_part1_nof_bits = 4U;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_csi](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_3_bits_over_csi)
{
  alloc_csi_opportunity(t_bench.get_main_ue(), default_csi_part1_bits);

  alloc_ded_harq_ack(t_bench.get_main_ue());
  alloc_ded_harq_ack(t_bench.get_main_ue());
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f2_for_csi, pri);

  pucch_expected_csi.uci_bits.harq_ack_nof_bits  = 3U;
  pucch_expected_csi.uci_bits.sr_bits            = sr_nof_bits::no_sr;
  pucch_expected_csi.uci_bits.csi_part1_nof_bits = 4U;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_csi](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}

TEST_F(pucch_alloc_format_0_test, test_harq_allocation_4_bits_over_csi)
{
  alloc_csi_opportunity(t_bench.get_main_ue(), default_csi_part1_bits);

  alloc_ded_harq_ack(t_bench.get_main_ue());
  alloc_ded_harq_ack(t_bench.get_main_ue());
  alloc_ded_harq_ack(t_bench.get_main_ue());
  auto pri = alloc_ded_harq_ack(t_bench.get_main_ue());
  ASSERT_TRUE(pri.has_value());
  ASSERT_EQ(pucch_res_idx_f2_for_csi, pri);

  pucch_expected_csi.uci_bits.harq_ack_nof_bits  = 4U;
  pucch_expected_csi.uci_bits.sr_bits            = sr_nof_bits::no_sr;
  pucch_expected_csi.uci_bits.csi_part1_nof_bits = 4U;
  ASSERT_EQ(1U, default_slot_grid.result.ul.pucchs.size());
  ASSERT_TRUE(find_pucch_pdu(default_slot_grid.result.ul.pucchs, [&expected = pucch_expected_csi](const auto& pdu) {
    return pucch_info_match(expected, pdu);
  }));
}
