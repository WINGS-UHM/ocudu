/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/fapi/p7/builders/ul_tti_request_builder.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi;

TEST(ul_tti_request_builder, valid_basic_parameters_passes)
{
  auto     scs        = subcarrier_spacing::kHz30;
  unsigned sfn        = 599;
  unsigned slot_index = 13;
  auto     slot       = slot_point(scs, sfn, slot_index);

  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  builder.set_basic_parameters(slot);

  ASSERT_EQ(slot, msg.slot);
  ASSERT_TRUE(msg.pdus.empty());
}

TEST(ul_tti_request_builder, add_pucch_f0_pdu_passes)
{
  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);

  rnti_t       rnti   = to_rnti(3);
  pucch_format format = pucch_format::FORMAT_0;

  builder.add_pucch_pdu(rnti, format);

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(ul_pdu_type::PUCCH, msg.pdus.back().pdu_type);
  const auto& pdu = msg.pdus.back().pucch_pdu;
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(format, pdu.format_type);
}

TEST(ul_tti_request_builder, add_pucch_f1_pdu_passes)
{
  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);

  rnti_t       rnti   = to_rnti(3);
  pucch_format format = pucch_format::FORMAT_1;

  builder.add_pucch_pdu(rnti, format);

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(ul_pdu_type::PUCCH, msg.pdus.back().pdu_type);
  const auto& pdu = msg.pdus.back().pucch_pdu;
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(format, pdu.format_type);
}

TEST(ul_tti_request_builder, add_pucch_f2_pdu_passes)
{
  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);

  rnti_t       rnti   = to_rnti(3);
  pucch_format format = pucch_format::FORMAT_2;

  builder.add_pucch_pdu(rnti, format);

  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);
  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(ul_pdu_type::PUCCH, msg.pdus.back().pdu_type);
  const auto& pdu = msg.pdus.back().pucch_pdu;
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(format, pdu.format_type);
}

TEST(ul_tti_request_builder, add_pucch_f3_pdu_passes)
{
  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);

  rnti_t       rnti   = to_rnti(3);
  pucch_format format = pucch_format::FORMAT_3;

  builder.add_pucch_pdu(rnti, format);

  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);
  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(ul_pdu_type::PUCCH, msg.pdus.back().pdu_type);
  const auto& pdu = msg.pdus.back().pucch_pdu;
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(format, pdu.format_type);
}

TEST(ul_tti_request_builder, add_pucch_f4_pdu_passes)
{
  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);

  rnti_t       rnti   = to_rnti(3);
  pucch_format format = pucch_format::FORMAT_4;

  builder.add_pucch_pdu(rnti, format);

  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format01)]);
  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_tti_request::pdu_type::PUCCH_format234)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(ul_pdu_type::PUCCH, msg.pdus.back().pdu_type);
  const auto& pdu = msg.pdus.back().pucch_pdu;
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(0, pdu.handle);
  ASSERT_EQ(format, pdu.format_type);
}

TEST(ul_tti_request_builder, add_pusch_pdu_passes)
{
  ul_tti_request         msg;
  ul_tti_request_builder builder(msg);

  ASSERT_TRUE(msg.pdus.empty());
  ASSERT_EQ(0U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_pdu_type::PUSCH)]);

  rnti_t rnti = to_rnti(3);

  builder.add_pusch_pdu(rnti);

  ASSERT_EQ(1U, msg.num_pdus_of_each_type[static_cast<unsigned>(ul_pdu_type::PUSCH)]);
  ASSERT_EQ(1U, msg.pdus.size());
  ASSERT_EQ(ul_pdu_type::PUSCH, msg.pdus.back().pdu_type);
  const auto& pdu = msg.pdus.back().pusch_pdu;
  ASSERT_EQ(rnti, pdu.rnti);
  ASSERT_EQ(0, pdu.handle);
}
