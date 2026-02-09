/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/rach_indication_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(rach_indication_builder, valid_basic_parameters_passes)
{
  rach_indication         msg;
  rach_indication_builder builder(msg);

  auto                 scs        = subcarrier_spacing::kHz30;
  unsigned             sfn        = 13;
  unsigned             slot_index = 12;
  auto                 slot       = slot_point(scs, sfn, slot_index);
  unsigned             symb_id    = 3;
  unsigned             slot_id    = 45;
  unsigned             ra_id_d    = 4;
  unsigned             index      = 32;
  std::optional<float> rssi;
  std::optional<float> snr = 10;

  builder.set_slot_point(slot);
  auto pdu_builder = builder.add_pdu(symb_id, slot_id, ra_id_d, rssi, snr);

  std::optional<phy_time_unit> timing         = phy_time_unit::from_seconds(0);
  std::optional<float>         preamble_power = 0;
  std::optional<float>         preamble_snr;

  pdu_builder.add_preamble(index, timing, preamble_power, preamble_snr);

  ASSERT_EQ(slot, msg.slot);
  ASSERT_EQ(1, msg.pdus.size());

  const auto& pdu = msg.pdus.back();
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(symb_id, pdu.symbol_index);
  ASSERT_EQ(slot_id, pdu.slot_index);
  ASSERT_EQ(ra_id_d, pdu.ra_index);
  ASSERT_EQ(rssi ? static_cast<unsigned>((rssi.value() + 140.F) * 1000) : std::numeric_limits<uint32_t>::max(),
            pdu.avg_rssi);
  ASSERT_EQ(snr ? (snr.value() + 64) * 2 : std::numeric_limits<uint8_t>::max(), pdu.avg_snr);
  ASSERT_EQ(1, pdu.preambles.size());

  const auto& preamble = pdu.preambles.back();
  ASSERT_EQ(index, preamble.preamble_index);
  ASSERT_EQ(timing, preamble.timing_advance_offset);
  ASSERT_EQ(preamble_power ? static_cast<unsigned>((preamble_power.value() + 140.F) * 1000.F)
                           : std::numeric_limits<uint32_t>::max(),
            preamble.preamble_pwr);
  ASSERT_EQ(preamble_snr ? static_cast<unsigned>((preamble_snr.value() + 64.F) * 2.F)
                         : std::numeric_limits<uint8_t>::max(),
            preamble.preamble_snr);
}
