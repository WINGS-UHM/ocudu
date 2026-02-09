/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/rx_data_indication_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(rx_data_indication_builder, valid_basic_parameters_passes)
{
  auto     scs        = subcarrier_spacing::kHz240;
  unsigned sfn        = 600;
  unsigned slot_index = 40;
  auto     slot       = slot_point(scs, sfn, slot_index);

  rx_data_indication         msg;
  rx_data_indication_builder builder(msg);

  builder.set_slot_point(slot);

  ASSERT_EQ(slot, msg.slot);
}

TEST(rx_data_indication_builder, add_custom_pdu_passes)
{
  static_vector<uint8_t, 18> transport_block = {1, 2, 3, 4, 5};

  rnti_t    rnti = to_rnti(29);
  harq_id_t harq = to_harq_id(14);

  rx_data_indication         msg;
  rx_data_indication_builder builder(msg);

  builder.add_pdu(rnti, harq, {transport_block});

  const auto& pdu = msg.pdus.back();
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(harq, pdu.harq_id);
  ASSERT_EQ(transport_block.size(), pdu.transport_block.size());
  ASSERT_EQ(transport_block.data(), pdu.transport_block.data());
}
