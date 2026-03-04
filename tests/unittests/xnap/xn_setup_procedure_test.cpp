// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "lib/xnap/procedures/xn_setup_procedure_asn1_helpers.h"
#include "xnap_test_helpers.h"
#include "ocudu/ran/cause/xnap_cause.h"
#include "ocudu/support/async/async_test_utils.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocudu::ocucp;

/// Fixture class for XNAP Setup tests.
class xn_setup_procedure_test : public xnap_test
{
protected:
  /// Peer configuration.
  gnb_id_t               peer_gnb_id             = {1, 22};
  plmn_identity          peer_plmn               = plmn_identity::test_value();
  tac_t                  peer_tac                = {7};
  s_nssai_t              peer_slice              = {};
  std::vector<s_nssai_t> peer_slice_support_list = {peer_slice};

  xnap_configuration xnap_peer_cfg = {
      peer_gnb_id,
      std::vector<supported_tracking_area>{{peer_tac, std::vector<plmn_item>{{peer_plmn, peer_slice_support_list}}}},
      std::vector<guami_t>{{peer_plmn, 1}},
  };
};

TEST_F(xn_setup_procedure_test, when_correct_setup_received_from_peer_setup_complete_is_sent)
{
  // Conect TX notifier to the XNAP instance, so that we can capture the response to the setup request.
  init_sctp_association();

  xnap_message xn_setup_req = generate_asn1_xn_setup_request(xnap_peer_cfg);
  xnap->handle_message(xn_setup_req);

  // Check XN setup response.
  xnap_message rep = get_last_message();
  ASSERT_EQ(rep.pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::successful_outcome);
  ASSERT_EQ(rep.pdu.successful_outcome().value.type(),
            asn1::xnap::xnap_elem_procs_o::successful_outcome_c::types_opts::xn_setup_resp);
}

/// Test the XN setup procedure timeout
TEST_F(xnap_test, when_xn_setup_procedure_times_out_then_setup_failure_is_returned)
{
  // Action 1: Launch XN setup procedure
  logger.info("Launch xn setup request procedure...");
  async_task<bool>         t = xnap->handle_xn_setup_request_required();
  lazy_task_launcher<bool> t_launcher(t);

  ASSERT_FALSE(t.ready());

  // Check XN setup request.
  xnap_message setup_req = xnc_gw.get_last_tx_message();
  ASSERT_EQ(setup_req.pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(setup_req.pdu.init_msg().value.type(),
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request);

  // Status: Fail XN setup procedure (XN-C peer doesn't respond).
  this->tick(t, std::chrono::milliseconds(5000));

  ASSERT_TRUE(t.ready());
  ASSERT_FALSE(t.get());
}

TEST_F(xn_setup_procedure_test,
       when_xn_setup_failure_with_time_to_wait_received_then_setup_is_retried_after_time_to_wait)
{
  // Action 1: Launch XN setup procedure
  logger.info("Launch xn setup request procedure...");
  async_task<bool>         t = xnap->handle_xn_setup_request_required();
  lazy_task_launcher<bool> t_launcher(t);

  ASSERT_FALSE(t.ready());

  // Check XN setup request.
  xnap_message setup_req = xnc_gw.get_last_tx_message();
  ASSERT_EQ(setup_req.pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(setup_req.pdu.init_msg().value.type(),
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request);

  // Action 2: Send XN setup failure with time to wait from peer.
  xnap_message setup_fail =
      generate_asn1_xn_setup_failure(xnap_cause_radio_network_t::unspecified, asn1::xnap::time_to_wait_e::v10s);
  xnap->handle_message(setup_fail);

  // Status: XN-C peer does not receive new XN Setup Request until time-to-wait has ended.
  this->tick(t, std::chrono::milliseconds(10000));

  // Check XN setup request is sent again.
  setup_req = xnc_gw.get_last_tx_message();
  ASSERT_EQ(setup_req.pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(setup_req.pdu.init_msg().value.type(),
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request);

  // Action 3: Send XN setup response from peer.
  xnap_message setup_resp = generate_asn1_xn_setup_response(xnap_peer_cfg);
  xnap->handle_message(setup_resp);

  // Check procedure completion.
  ASSERT_TRUE(t.ready());
  ASSERT_TRUE(t.get());
}

TEST_F(xn_setup_procedure_test, when_xn_setup_failure_without_time_to_wait_received_then_setup_fails)
{
  // Action 1: Launch XN setup procedure
  logger.info("Launch xn setup request procedure...");
  async_task<bool>         t = xnap->handle_xn_setup_request_required();
  lazy_task_launcher<bool> t_launcher(t);

  ASSERT_FALSE(t.ready());

  // Check XN setup request.
  xnap_message setup_req = xnc_gw.get_last_tx_message();
  ASSERT_EQ(setup_req.pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(setup_req.pdu.init_msg().value.type(),
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request);

  // Action 2: Send XN setup failure without time to wait from peer.
  xnap_message setup_fail = generate_asn1_xn_setup_failure(xnap_cause_radio_network_t::unspecified);
  xnap->handle_message(setup_fail);

  // Check procedure completion.
  ASSERT_TRUE(t.ready());
  ASSERT_FALSE(t.get());
}

TEST_F(xn_setup_procedure_test, when_xn_setup_request_required_then_setup_is_sent_to_peer)
{
  // Action 1: Launch XN setup procedure
  logger.info("Launch xn setup request procedure...");
  async_task<bool>         t = xnap->handle_xn_setup_request_required();
  lazy_task_launcher<bool> t_launcher(t);

  ASSERT_FALSE(t.ready());

  // Check XN setup request.
  xnap_message setup_req = xnc_gw.get_last_tx_message();
  ASSERT_EQ(setup_req.pdu.type(), asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(setup_req.pdu.init_msg().value.type(),
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request);

  // Action 2: Send XN setup response from peer.
  xnap_message setup_resp = generate_asn1_xn_setup_response(xnap_peer_cfg);
  xnap->handle_message(setup_resp);

  // Check procedure completion.
  ASSERT_TRUE(t.ready());
  ASSERT_TRUE(t.get());
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
