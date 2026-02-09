/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "message_builder_helpers.h"
#include "pucch.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi_adaptor;
using namespace unittest;

static void check_context_parameters(const ul_pucch_context&   context,
                                     const fapi::ul_pucch_pdu& fapi_pdu,
                                     slot_point                slot,
                                     pucch_format              format)
{
  ASSERT_EQ(slot, context.slot);
  ASSERT_EQ(fapi_pdu.rnti, context.rnti);
  ASSERT_EQ(fapi_pdu.format_type, context.format);
}

static void check_context_f0_or_f1_parameters(const ul_pucch_context& context, const fapi::ul_pucch_pdu& fapi_pdu)
{
  ASSERT_TRUE(context.context_f0_f1.has_value());
  const ul_pucch_f0_f1_context& ctx = context.context_f0_f1.value();
  ASSERT_EQ(fapi_pdu.sr_bit_len == 1, ctx.is_sr_opportunity);
}

TEST(FAPIPPHYULPUCCHAdaptorTest, ValidFormat1PDUPass)
{
  fapi::ul_pucch_pdu fapi_pdu = build_valid_ul_pucch_f1_pdu();

  auto     scs             = to_numerology_value(fapi_pdu.scs);
  unsigned sfn             = 1U;
  unsigned slot_index      = 2U;
  auto     slot            = slot_point(scs, sfn, slot_index);
  unsigned nof_rx_antennas = 1U;

  uplink_pdu_slot_repository::pucch_pdu pdu;
  convert_pucch_fapi_to_phy(pdu, fapi_pdu, slot, nof_rx_antennas);

  // Format 1 custom parameters.
  const auto& phy_pdu = std::get<pucch_processor::format1_configuration>(pdu.config);
  ASSERT_EQ(fapi_pdu.symbols.start(), phy_pdu.start_symbol_index);
  ASSERT_EQ(fapi_pdu.symbols.length(), phy_pdu.nof_symbols);
  ASSERT_EQ(fapi_pdu.time_domain_occ_index, phy_pdu.time_domain_occ);
  ASSERT_EQ(fapi_pdu.initial_cyclic_shift, phy_pdu.initial_cyclic_shift);

  // Common parameters.
  ASSERT_EQ(slot, phy_pdu.slot);
  ASSERT_EQ(fapi_pdu.bwp.start(), phy_pdu.bwp_start_rb);
  ASSERT_EQ(fapi_pdu.bwp.length(), phy_pdu.bwp_size_rb);
  ASSERT_EQ(fapi_pdu.cp, phy_pdu.cp);

  ASSERT_EQ(fapi_pdu.prbs.start(), phy_pdu.starting_prb);
  if (phy_pdu.second_hop_prb.has_value()) {
    ASSERT_EQ(fapi_pdu.second_hop_prb, phy_pdu.second_hop_prb.value());
  }
  ASSERT_EQ(fapi_pdu.bit_len_harq, phy_pdu.nof_harq_ack);
  ASSERT_EQ(fapi_pdu.nid_pucch_hopping, phy_pdu.n_id);

  // Ports.
  ASSERT_EQ(nof_rx_antennas, phy_pdu.ports.size());
  for (unsigned i = 0; i != nof_rx_antennas; ++i) {
    ASSERT_EQ(i, phy_pdu.ports[i]);
  }

  // Context parameters.
  check_context_parameters(pdu.context, fapi_pdu, slot, pucch_format::FORMAT_1);

  // Context Format 0 or Format 1 parameters.
  check_context_f0_or_f1_parameters(pdu.context, fapi_pdu);
}
