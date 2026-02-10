/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/xnap/procedures/xn_setup_procedure_asn1_helpers.h"
#include "lib/xnap/xnap_impl.h"
#include "ocudu/support/executors/manual_task_worker.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocudu::ocucp;

/// Dummy class to check for XN TX messages.
class dummy_xnap_message_notifier : public xnap_message_notifier
{
public:
  ~dummy_xnap_message_notifier() override = default;

  bool on_new_message(const xnap_message& msg) override
  {
    last_msg = msg;
    return true;
  }
  xnap_message last_msg;
};

/// Fixture class for XNAP Setup tests
class xn_setup_procedure_test : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Init test's loggers.
    ocudulog::init();
    logger.set_level(ocudulog::basic_levels::debug);

    ocudulog::fetch_basic_logger("XNAP", false).set_level(ocudulog::basic_levels::debug);
    ocudulog::fetch_basic_logger("XNAP", false).set_hex_dump_max_size(100);

    auto assoc = std::make_unique<dummy_xnap_message_notifier>();
    xnap       = std::make_unique<xnap_impl>(xnap_local_cfg, ctrl_worker);
    tx_assoc   = assoc.get();
    xnap->set_tx_association_notifier(std::move(assoc));
  }

  void TearDown() override
  {
    // Flush logger after each test.
    ocudulog::flush();
    tx_assoc = nullptr;
  }

  ocudulog::basic_logger&    logger = ocudulog::fetch_basic_logger("TEST", false);
  manual_task_worker         ctrl_worker{128};
  std::unique_ptr<xnap_impl> xnap                     = nullptr;
  gnb_id_t                   local_gnb_id             = {0, 22};
  plmn_identity              local_plmn               = plmn_identity::test_value();
  tac_t                      local_tac                = {8};
  s_nssai_t                  local_slice              = {};
  std::vector<s_nssai_t>     local_slice_support_list = {local_slice};

  xnap_configuration xnap_local_cfg = {
      local_gnb_id,
      std::vector<supported_tracking_area>{
          {local_tac, std::vector<plmn_item>{{local_plmn, local_slice_support_list}}}}};

  /// Peer configuration.
  gnb_id_t               peer_gnb_id             = {1, 22};
  plmn_identity          peer_plmn               = plmn_identity::test_value();
  tac_t                  peer_tac                = {7};
  s_nssai_t              peer_slice              = {};
  std::vector<s_nssai_t> peer_slice_support_list = {peer_slice};

  xnap_configuration xnap_peer_cfg = {
      peer_gnb_id,
      std::vector<supported_tracking_area>{{peer_tac, std::vector<plmn_item>{{peer_plmn, peer_slice_support_list}}}}};

  std::optional<xnap_message> get_last_message()
  {
    if (tx_assoc == nullptr) {
      return std::nullopt;
    }
    return tx_assoc->last_msg;
  }

private:
  dummy_xnap_message_notifier* tx_assoc;
};

TEST_F(xn_setup_procedure_test, when_correct_setup_received_from_peer_setup_complete_is_sent)
{
  xnap_message xn_setup_req = generate_xn_setup_request(xnap_peer_cfg);
  xnap->handle_message(xn_setup_req);
  std::optional<xnap_message> rep = get_last_message();

  // Check XN setup response.
  ASSERT_TRUE(rep.has_value());
  ASSERT_EQ(rep->pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::successful_outcome);
  ASSERT_EQ(rep->pdu.successful_outcome().value.type(),
            asn1::xnap::xnap_elem_procs_o::successful_outcome_c::types_opts::xn_setup_resp);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
