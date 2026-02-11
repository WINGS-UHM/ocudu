/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mac_to_fapi_fastpath_translator.h"
#include "pdu_translators/helpers.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"
#include "ocudu/fapi/p7/messages/tx_data_request.h"
#include "ocudu/fapi/p7/messages/ul_dci_request.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/fapi/p7/p7_last_request_notifier.h"
#include "ocudu/fapi/p7/p7_requests_gateway.h"
#include "ocudu/fapi_adaptor/precoding_matrix_table_generator.h"
#include "ocudu/fapi_adaptor/uci_part2_correspondence_generator.h"
#include "ocudu/ocudulog/ocudulog.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi_adaptor;

namespace {

/// Spy implementation of a P7 requests gateway.
class p7_requests_gateway_spy : public fapi::p7_requests_gateway
{
  bool                  has_dl_tti_request_method_been_called = false;
  fapi::dl_tti_request  dl_tti_msg;
  bool                  has_ul_tti_request_method_been_called = false;
  fapi::ul_tti_request  ul_tti_msg;
  bool                  has_tx_data_request_method_been_called = false;
  fapi::tx_data_request tx_data_msg;
  bool                  has_ul_dci_request_method_been_called = false;
  fapi::ul_dci_request  ul_dci_msg;

public:
  bool has_dl_tti_request_method_called() const { return has_dl_tti_request_method_been_called; }
  bool has_ul_tti_request_method_called() const { return has_ul_tti_request_method_been_called; }
  bool has_tx_data_request_method_called() const { return has_tx_data_request_method_been_called; }
  bool has_ul_dci_request_method_called() const { return has_ul_dci_request_method_been_called; }
  const fapi::dl_tti_request&  dl_tti_request_msg() const { return dl_tti_msg; }
  const fapi::ul_tti_request&  ul_tti_request_msg() const { return ul_tti_msg; }
  const fapi::tx_data_request& tx_data_request_msg() const { return tx_data_msg; }
  const fapi::ul_dci_request&  ul_dci_request_msg() const { return ul_dci_msg; }

  void send_dl_tti_request(const fapi::dl_tti_request& msg) override
  {
    has_dl_tti_request_method_been_called = true;
    dl_tti_msg                            = msg;
  }

  void send_ul_tti_request(const fapi::ul_tti_request& msg) override
  {
    has_ul_tti_request_method_been_called = true;
    ul_tti_msg                            = msg;
  }

  void send_ul_dci_request(const fapi::ul_dci_request& msg) override
  {
    has_ul_dci_request_method_been_called = true;
    ul_dci_msg                            = msg;
  }

  void send_tx_data_request(const fapi::tx_data_request& msg) override
  {
    has_tx_data_request_method_been_called = true;
    tx_data_msg                            = msg;
  }
};

class slot_last_message_notifier_spy : public fapi::p7_last_request_notifier
{
  bool       has_been_notified = false;
  slot_point notif_slot;

public:
  void on_last_message(slot_point slot) override
  {
    has_been_notified = true;
    notif_slot        = slot;
  }

  bool       has_on_last_message_method_called() const { return has_been_notified; }
  slot_point slot() const { return notif_slot; }
};

class mac_to_fapi_translator_fixture : public ::testing::Test
{
protected:
  p7_requests_gateway_spy                                                                           gateway_spy;
  slot_last_message_notifier_spy                                                                    notifier_spy;
  const unsigned                                                                                    nof_prbs = 51U;
  std::pair<std::unique_ptr<precoding_matrix_mapper>, std::unique_ptr<precoding_matrix_repository>> pm_tools =
      generate_precoding_matrix_tables(1, 0);
  std::pair<std::unique_ptr<uci_part2_correspondence_mapper>, std::unique_ptr<uci_part2_correspondence_repository>>
                                  uci_part2_tools = generate_uci_part2_correspondence(1);
  mac_to_fapi_fastpath_translator translator;

  mac_to_fapi_translator_fixture() :
    translator({nof_prbs},
               {gateway_spy, notifier_spy, std::move(std::get<0>(pm_tools)), std::move(std::get<0>(uci_part2_tools))})
  {
  }
};

} // namespace

TEST_F(mac_to_fapi_translator_fixture, valid_dl_sched_results_generate_correct_dl_tti_request)
{
  ASSERT_FALSE(gateway_spy.has_dl_tti_request_method_called());

  const unittests::mac_dl_sched_result_test_helper& result_test = ocudu::unittests::build_valid_mac_dl_sched_result();
  const mac_dl_sched_result&                        result      = result_test.result;
  translator.on_new_downlink_scheduler_results(result);

  ASSERT_TRUE(gateway_spy.has_dl_tti_request_method_called());
  const fapi::dl_tti_request& msg = gateway_spy.dl_tti_request_msg();
  ASSERT_EQ(msg.pdus.size(), 7U);
  ASSERT_EQ((msg.pdus.begin() + 1)->pdu_type, fapi::dl_pdu_type::PDCCH);
  ASSERT_EQ(msg.pdus.back().pdu_type, fapi::dl_pdu_type::PDSCH);
  ASSERT_EQ(msg.pdus.front().pdu_type, fapi::dl_pdu_type::PDCCH);
  ASSERT_EQ((msg.pdus.end() - 2)->pdu_type, fapi::dl_pdu_type::SSB);
  ASSERT_EQ((msg.pdus.end() - 3)->pdu_type, fapi::dl_pdu_type::SSB);
}

TEST_F(mac_to_fapi_translator_fixture, valid_ul_sched_results_generate_correct_ul_tti_request)
{
  ASSERT_FALSE(gateway_spy.has_ul_tti_request_method_called());

  const unittests::mac_ul_sched_result_test_helper& result_test = unittests::build_valid_mac_ul_sched_result();
  const mac_ul_sched_result&                        result      = result_test.result;
  translator.on_new_uplink_scheduler_results(result);

  ASSERT_TRUE(gateway_spy.has_ul_tti_request_method_called());
  const fapi::ul_tti_request& msg = gateway_spy.ul_tti_request_msg();
  ASSERT_EQ(msg.pdus.size(), 4U);
  ASSERT_EQ(msg.pdus.front().pdu_type, fapi::ul_pdu_type::PRACH);
  ASSERT_EQ((msg.pdus.begin() + 1)->pdu_type, fapi::ul_pdu_type::PUSCH);
  ASSERT_EQ((msg.pdus.end() - 2)->pdu_type, fapi::ul_pdu_type::PUCCH);
  ASSERT_EQ(msg.pdus.back().pdu_type, fapi::ul_pdu_type::PUCCH);
}

TEST_F(mac_to_fapi_translator_fixture, valid_dl_data_results_generate_correct_tx_data_request)
{
  ASSERT_FALSE(gateway_spy.has_tx_data_request_method_called());

  const unittests::mac_dl_sched_result_test_helper& result_test = unittests::build_valid_mac_dl_sched_result();
  const mac_dl_sched_result&                        result      = result_test.result;
  translator.on_new_downlink_scheduler_results(result);
  const unittests::mac_dl_data_result_test_helper& data_result = unittests::build_valid_mac_data_result();
  translator.on_new_downlink_data(data_result.result);

  ASSERT_TRUE(gateway_spy.has_tx_data_request_method_called());
  const fapi::tx_data_request& msg = gateway_spy.tx_data_request_msg();
  ASSERT_EQ(msg.pdus.size(), 1);
}

TEST_F(mac_to_fapi_translator_fixture, valid_dl_data_results_generate_correct_ul_dci_request)
{
  ASSERT_FALSE(gateway_spy.has_ul_dci_request_method_called());

  const unittests::mac_dl_sched_result_test_helper& result_test = unittests::build_valid_mac_dl_sched_result();
  const mac_dl_sched_result&                        result      = result_test.result;
  translator.on_new_downlink_scheduler_results(result);

  ASSERT_TRUE(gateway_spy.has_ul_dci_request_method_called());
  const fapi::ul_dci_request& msg = gateway_spy.ul_dci_request_msg();
  ASSERT_EQ(msg.pdus.size(), 1);
}

TEST_F(mac_to_fapi_translator_fixture, last_message_is_notified)
{
  slot_point slot(1, 1, 1);

  ASSERT_FALSE(notifier_spy.has_on_last_message_method_called());
  translator.on_cell_results_completion(slot);
  ASSERT_TRUE(notifier_spy.has_on_last_message_method_called());
  ASSERT_EQ(slot, notifier_spy.slot());
}

TEST_F(mac_to_fapi_translator_fixture, dl_tti_message_with_all_pdus_passes)
{
  const unittests::mac_dl_sched_result_test_helper& result_test =
      unittests::build_valid_mac_dl_sched_result_with_all_supported_pdus();
  const mac_dl_sched_result& result = result_test.result;
  translator.on_new_downlink_scheduler_results(result);

  const fapi::ul_dci_request& ul_dci_msg = gateway_spy.ul_dci_request_msg();
  ASSERT_EQ(ul_dci_msg.pdus.size(), MAX_UL_PDCCH_PDUS_PER_SLOT);

  const fapi::dl_tti_request& dl_tti_msg = gateway_spy.dl_tti_request_msg();
  // As the MAC struct does still not contain MAX_PRS_PDUS_PER_SLOT, substract the value.
  ASSERT_EQ(dl_tti_msg.pdus.size(), MAX_DL_PDUS_PER_SLOT - MAX_PRS_PDUS_PER_SLOT);
}

TEST_F(mac_to_fapi_translator_fixture, tx_data_message_with_all_pdus_passes)
{
  const unittests::mac_dl_sched_result_test_helper& result_test =
      unittests::build_valid_mac_dl_sched_result_with_all_supported_pdus();
  const mac_dl_sched_result& result = result_test.result;
  translator.on_new_downlink_scheduler_results(result);

  const unittests::mac_dl_data_result_test_helper& data_result =
      unittests::build_valid_mac_data_result_with_all_supported_pdu();
  translator.on_new_downlink_data(data_result.result);

  const fapi::tx_data_request& tx_data_msg = gateway_spy.tx_data_request_msg();
  ASSERT_EQ(tx_data_msg.pdus.size(),
            MAX_SI_PDUS_PER_SLOT + MAX_RAR_PDUS_PER_SLOT + MAX_UE_PDUS_PER_SLOT + MAX_PAGING_PDUS_PER_SLOT);

  const fapi::ul_dci_request& ul_dci_msg = gateway_spy.ul_dci_request_msg();
  ASSERT_EQ(ul_dci_msg.pdus.size(), MAX_UL_PDCCH_PDUS_PER_SLOT);

  const fapi::dl_tti_request& dl_tti_msg = gateway_spy.dl_tti_request_msg();
  // As the MAC struct does still not contain MAX_PRS_PDUS_PER_SLOT, substract the value.
  ASSERT_EQ(dl_tti_msg.pdus.size(), MAX_DL_PDUS_PER_SLOT - MAX_PRS_PDUS_PER_SLOT);
}

TEST_F(mac_to_fapi_translator_fixture, ul_tti_message_with_all_pdus_passes)
{
  ocudulog::fetch_basic_logger("FAPI").set_level(ocudulog::basic_levels::debug);

  ASSERT_FALSE(gateway_spy.has_ul_tti_request_method_called());

  const unittests::mac_ul_sched_result_test_helper& result_test =
      unittests::build_valid_mac_ul_sched_result_with_all_supported_pdus();
  const mac_ul_sched_result& result = result_test.result;
  translator.on_new_uplink_scheduler_results(result);

  const fapi::ul_tti_request& msg = gateway_spy.ul_tti_request_msg();
  ASSERT_EQ(msg.pdus.size(), MAX_UL_PDUS_PER_SLOT);
}
