/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/ul_dci_request_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(ul_dci_request_builder, valid_basic_parameters_passes)
{
  ul_dci_request         msg;
  ul_dci_request_builder builder(msg);

  auto     scs        = subcarrier_spacing::kHz30;
  unsigned sfn        = 16;
  unsigned slot_index = 18;
  auto     slot       = slot_point(scs, sfn, slot_index);

  builder.set_slot(slot);

  for (unsigned i = 0, e = 10; i != e; ++i) {
    builder.add_pdcch_pdu();

    ASSERT_EQ(i + 1, msg.pdus.size());
  }

  ASSERT_EQ(slot, msg.slot);
}
