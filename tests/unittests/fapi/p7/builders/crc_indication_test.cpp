/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/crc_indication_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(crc_indication_builder, valid_indication_passes)
{
  for (unsigned i = 0; i != 2; ++i) {
    crc_indication         msg;
    crc_indication_builder builder(msg);

    auto     scs        = subcarrier_spacing::kHz30;
    unsigned sfn        = 100;
    unsigned slot_index = 15;
    auto     slot       = slot_point(scs, sfn, slot_index);

    builder.set_basic_parameters(slot);

    rnti_t    rnti          = to_rnti(10);
    harq_id_t harq_id       = to_harq_id(0);
    uint8_t   tb_crc_status = 0;

    std::optional<float> ul_sinr_dB;
    ul_sinr_dB.emplace(-65);

    std::optional<phy_time_unit> timing_advance_offset;
    timing_advance_offset.emplace(phy_time_unit::from_seconds(30));

    std::optional<float> rssi_dB;
    rssi_dB.emplace(-64);

    std::optional<float> rsrp_dB;
    rsrp_dB.emplace(-100);
    bool use_dB = i;

    builder.add_pdu(rnti, harq_id, tb_crc_status, ul_sinr_dB, timing_advance_offset, rssi_dB, rsrp_dB, use_dB);

    ASSERT_EQ(slot, msg.slot);

    const crc_ind_pdu& pdu = msg.pdus.back();
    ASSERT_EQ(0, pdu.handle);
    ASSERT_EQ(harq_id, pdu.harq_id);
    ASSERT_EQ(rnti, pdu.rnti);
    ASSERT_EQ(tb_crc_status, pdu.tb_crc_status_ok);
    ASSERT_EQ(static_cast<int16_t>(ul_sinr_dB ? ul_sinr_dB.value() * 500.F : -32768), pdu.ul_sinr_metric);
    ASSERT_EQ(timing_advance_offset ? timing_advance_offset.value() : phy_time_unit(), pdu.timing_advance_offset);
    ASSERT_EQ(static_cast<uint16_t>(rssi_dB ? (rssi_dB.value() + 128.F) * 10.F : 65535), pdu.rssi);
    ASSERT_EQ(static_cast<uint16_t>(rsrp_dB ? (rsrp_dB.value() + (use_dB ? 140.F : 128.F)) * 10.F : 65535), pdu.rsrp);
  }
}

TEST(crc_indication_builder, valid_indication_with_no_metrics_passes)
{
  crc_indication         msg;
  crc_indication_builder builder(msg);

  auto     scs        = subcarrier_spacing::kHz30;
  unsigned sfn        = 100;
  unsigned slot_index = 15;
  auto     slot       = slot_point(scs, sfn, slot_index);

  builder.set_basic_parameters(slot);

  rnti_t    rnti          = to_rnti(10);
  harq_id_t harq_id       = to_harq_id(0);
  uint8_t   tb_crc_status = 0;

  std::optional<float>         ul_sinr_dB;
  std::optional<phy_time_unit> timing_advance_offset;
  std::optional<float>         rssi_dB;
  std::optional<float>         rsrp_dB;

  builder.add_pdu(rnti, harq_id, tb_crc_status, ul_sinr_dB, timing_advance_offset, rssi_dB, rsrp_dB);

  ASSERT_EQ(slot, msg.slot);

  const crc_ind_pdu& pdu = msg.pdus.back();
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(harq_id, pdu.harq_id);
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(tb_crc_status, pdu.tb_crc_status_ok);
  ASSERT_EQ(static_cast<int16_t>(ul_sinr_dB ? ul_sinr_dB.value() * 500.F : -32768), pdu.ul_sinr_metric);
  ASSERT_EQ(timing_advance_offset, pdu.timing_advance_offset);
  ASSERT_EQ(static_cast<uint16_t>(rssi_dB ? (rssi_dB.value() + 128) * 10.F : 65535), pdu.rssi);
  ASSERT_EQ(static_cast<uint16_t>(ul_sinr_dB ? ul_sinr_dB.value() * 500.F : 65535), pdu.rsrp);
}
