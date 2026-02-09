/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/dl_tti_request_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(dl_ssb_pdu_builder, valid_slot_point_passes)
{
  auto     scs        = subcarrier_spacing::kHz30;
  unsigned sfn        = 599;
  unsigned slot_index = 13;
  auto     slot       = slot_point(scs, sfn, slot_index);

  dl_tti_request         msg;
  dl_tti_request_builder builder(msg);

  builder.set_slot(slot);

  ASSERT_EQ(slot, msg.slot);
  ASSERT_TRUE(msg.pdus.empty());
}

TEST(dl_ssb_pdu_builder, add_pdcch_pdu_passes)
{
  dl_tti_request         msg;
  dl_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::PDCCH)]);

  builder.add_pdcch_pdu();

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::PDCCH)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(dl_pdu_type::PDCCH, msg.pdus.back().pdu_type);
  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_tti_request::DL_DCI_INDEX)]);
}

TEST(dl_ssb_pdu_builder, add_pdsch_pdu_passes)
{
  dl_tti_request         msg;
  dl_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::PDSCH)]);

  builder.add_pdsch_pdu();

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::PDSCH)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(dl_pdu_type::PDSCH, msg.pdus.back().pdu_type);
}

TEST(dl_ssb_pdu_builder, add_ssb_pdu_passes)
{
  dl_tti_request         msg;
  dl_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::SSB)]);

  builder.add_ssb_pdu();

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::SSB)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(dl_pdu_type::SSB, msg.pdus.back().pdu_type);
}

TEST(dl_ssb_pdu_builder, add_csi_pdu_passes)
{
  dl_tti_request         msg;
  dl_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::CSI_RS)]);

  uint16_t                  start_rb      = 3;
  uint16_t                  nof_rbs       = 4;
  csi_rs_type               type          = csi_rs_type::CSI_RS_NZP;
  uint8_t                   row           = 3;
  bounded_bitset<12, false> freq_domain   = {0, 0, 1};
  uint8_t                   symb_l0       = 5;
  uint8_t                   symb_l1       = 2;
  csi_rs_cdm_type           cdm_type      = csi_rs_cdm_type::cdm8_FD2_TD4;
  csi_rs_freq_density_type  freq_density  = csi_rs_freq_density_type::dot5_even_RB;
  uint16_t                  scrambling_id = 56;

  auto csi_builder = builder.add_csi_rs_pdu();

  csi_builder.set_csi_resource_config_parameters(type, row, cdm_type, scrambling_id)
      .set_frequency_domain_parameters(freq_domain, freq_density)
      .set_time_domain_parameters(symb_l0, symb_l1)
      .set_resource_block_parameters({start_rb, nof_rbs});

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(dl_pdu_type::CSI_RS)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(dl_pdu_type::CSI_RS, msg.pdus.back().pdu_type);
}
