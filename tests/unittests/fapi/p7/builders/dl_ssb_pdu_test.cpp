/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/dl_ssb_pdu_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(dl_ssb_pdu_builder, valid_basic_parameters_passes)
{
  dl_ssb_pdu         pdu;
  dl_ssb_pdu_builder builder(pdu);

  pci_t                 pci               = 106;
  beta_pss_profile_type pss_profile       = beta_pss_profile_type::dB_0;
  auto                  block_index       = ssb_id_t(3);
  ssb_subcarrier_offset subcarrier_offset = 2;
  unsigned              offset_pointA     = 39;
  ssb_pattern_case      case_type         = ssb_pattern_case::A;
  subcarrier_spacing    scs               = subcarrier_spacing::kHz15;
  unsigned              L_max             = 8;

  builder.set_carrier_parameters(scs)
      .set_cell_parameters(pci)
      .set_power_parameters(pss_profile)
      .set_ssb_parameters(block_index, subcarrier_offset, offset_pointA, case_type, L_max);

  ASSERT_EQ(pci, pdu.phys_cell_id);
  ASSERT_EQ(pss_profile, pdu.beta_pss_profile_nr);
  ASSERT_EQ(block_index, pdu.ssb_block_index);
  ASSERT_EQ(subcarrier_offset, pdu.subcarrier_offset);
  ASSERT_EQ(offset_pointA, pdu.ssb_offset_pointA.value());
  ASSERT_EQ(case_type, pdu.case_type);
  ASSERT_EQ(scs, pdu.scs);
  ASSERT_EQ(L_max, pdu.L_max);
}

TEST(dl_ssb_pdu_builder, valid_bch_payload_mixed_passes)
{
  dl_ssb_pdu         pdu;
  dl_ssb_pdu_builder builder(pdu);

  unsigned payload = 15453423;

  builder.set_bch_payload_phy_timing_info(payload);

  ASSERT_EQ(payload, pdu.bch_payload);
}
