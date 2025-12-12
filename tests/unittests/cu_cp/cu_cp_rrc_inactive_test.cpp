/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "cu_cp_test_environment.h"
#include "tests/test_doubles/e1ap/e1ap_test_message_validators.h"
#include "tests/test_doubles/f1ap/f1ap_test_message_validators.h"
#include "tests/test_doubles/ngap/ngap_test_message_validators.h"
#include "tests/test_doubles/rrc/rrc_test_message_validators.h"
#include "tests/unittests/e1ap/common/e1ap_cu_cp_test_messages.h"
#include "ocudu/e1ap/common/e1ap_message.h"
#include "ocudu/ngap/ngap_message.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocucp;

class cu_cp_rrc_inactive_test : public cu_cp_test_environment, public ::testing::Test
{
public:
  cu_cp_rrc_inactive_test() :
    cu_cp_test_environment({/* max nof cu-ups */ 8,
                            /* max nof dus */ 8,
                            /* max nof ues */ 8192,
                            /* max nof drbs per ue */ 8,
                            /* amf config */ {{default_supported_tracking_area}},
                            /* trigger ho from measurements */ true,
                            /* enable rrc inactive */ true})
  {
    // Run NG setup to completion.
    run_ng_setup();

    // Setup DU.
    std::optional<unsigned> ret = connect_new_du();
    EXPECT_TRUE(ret.has_value());
    du_idx = ret.value();
    EXPECT_TRUE(this->run_f1_setup(du_idx));

    // Setup CU-UP.
    ret = connect_new_cu_up();
    EXPECT_TRUE(ret.has_value());
    cu_up_idx = ret.value();
    EXPECT_TRUE(this->run_e1_setup(cu_up_idx));

    // Connect UE 0x4601.
    EXPECT_TRUE(attach_ue(du_idx, cu_up_idx, du_ue_id, crnti, amf_ue_id, cu_up_e1ap_id, psi, drb_id_t::drb1, qfi));
    ue_ctx = this->find_ue_context(du_idx, du_ue_id);
    EXPECT_NE(ue_ctx, nullptr);
  }

  [[nodiscard]] bool
  setup_pdu_session(pdu_session_id_t psi_,
                    drb_id_t         drb_id_,
                    qos_flow_id_t    qfi_,
                    byte_buffer      rrc_reconfiguration_complete = make_byte_buffer("00070e00cc6fcda5").value(),
                    bool             is_initial_session_          = true)
  {
    return cu_cp_test_environment::setup_pdu_session(du_idx,
                                                     cu_up_idx,
                                                     du_ue_id,
                                                     crnti,
                                                     cu_up_e1ap_id,
                                                     psi_,
                                                     drb_id_,
                                                     qfi_,
                                                     std::move(rrc_reconfiguration_complete),
                                                     is_initial_session_);
  }

  [[nodiscard]] bool send_bearer_context_inactivity_notification(const e1ap_message& inactivity_notification)
  {
    report_fatal_error_if_not(not this->get_amf().try_pop_rx_pdu(ngap_pdu),
                              "there are still NGAP messages to pop from AMF");
    report_fatal_error_if_not(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                              "there are still E1AP messages to pop from CU-UP");

    // Inject inactivity notification.
    get_cu_up(cu_up_idx).push_tx_pdu(inactivity_notification);
    return true;
  }

  [[nodiscard]] bool
  send_ue_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request()
  {
    report_fatal_error_if_not(not this->get_amf().try_pop_rx_pdu(ngap_pdu),
                              "there are still NGAP messages to pop from AMF");
    report_fatal_error_if_not(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                              "there are still E1AP messages to pop from CU-UP");

    // Inject inactivity notification and wait for Bearer Context Modification Request.
    if (!send_bearer_context_inactivity_notification(generate_bearer_context_inactivity_notification_with_ue_level(
            ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id))) {
      return false;
    }
    report_fatal_error_if_not(this->wait_for_e1ap_tx_pdu(cu_up_idx, e1ap_pdu),
                              "Failed to receive Bearer Context Modification Request");
    report_fatal_error_if_not(test_helpers::is_valid_bearer_context_modification_request(e1ap_pdu),
                              "Invalid Bearer Context Modification Request");
    return true;
  }

  [[nodiscard]] bool
  send_drb_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request()
  {
    report_fatal_error_if_not(not this->get_amf().try_pop_rx_pdu(ngap_pdu),
                              "there are still NGAP messages to pop from AMF");
    report_fatal_error_if_not(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                              "there are still E1AP messages to pop from CU-UP");

    // Inject inactivity notification and wait for Bearer Context Modification Request.
    if (!send_bearer_context_inactivity_notification(generate_bearer_context_inactivity_notification_with_drb_level(
            ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id, {}, {drb_id_t::drb1}))) {
      return false;
    }
    report_fatal_error_if_not(this->wait_for_e1ap_tx_pdu(cu_up_idx, e1ap_pdu),
                              "Failed to receive Bearer Context Modification Request");
    report_fatal_error_if_not(test_helpers::is_valid_bearer_context_modification_request(e1ap_pdu),
                              "Invalid Bearer Context Modification Request");
    return true;
  }

  [[nodiscard]] bool
  send_pdu_session_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request()
  {
    report_fatal_error_if_not(not this->get_amf().try_pop_rx_pdu(ngap_pdu),
                              "there are still NGAP messages to pop from AMF");
    report_fatal_error_if_not(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                              "there are still E1AP messages to pop from CU-UP");

    // Inject inactivity notification and wait for Bearer Context Modification Request.
    if (!send_bearer_context_inactivity_notification(
            generate_bearer_context_inactivity_notification_with_pdu_session_level(
                ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id, {}, {psi}))) {
      return false;
    }
    report_fatal_error_if_not(this->wait_for_e1ap_tx_pdu(cu_up_idx, e1ap_pdu),
                              "Failed to receive Bearer Context Modification Request");
    report_fatal_error_if_not(test_helpers::is_valid_bearer_context_modification_request(e1ap_pdu),
                              "Invalid Bearer Context Modification Request");
    return true;
  }

  [[nodiscard]] bool send_bearer_context_modification_response_and_await_ue_context_release_command()
  {
    // Inject Bearer Context Modification Response and wait for PDU Session Resource Setup Response.
    get_cu_up(cu_up_idx).push_tx_pdu(generate_bearer_context_modification_response(
        ue_ctx->cu_cp_e1ap_id.value(), ue_ctx->cu_up_e1ap_id.value(), {}, {}));
    report_fatal_error_if_not(this->wait_for_f1ap_tx_pdu(du_idx, f1ap_pdu),
                              "Failed to receive UE Context Release Command");
    report_fatal_error_if_not(test_helpers::is_valid_ue_context_release_command(f1ap_pdu),
                              "Invalid UE Context Release Command");

    const byte_buffer& rrc_container = test_helpers::get_rrc_container(f1ap_pdu);

    report_fatal_error_if_not(
        test_helpers::is_valid_rrc_release_with_suspend(test_helpers::extract_dl_dcch_msg(rrc_container)),
        "Invalid RRC Release");

    return true;
  }

  [[nodiscard]] bool send_f1ap_ue_context_release_complete()
  {
    // Inject F1AP UE Context Release Complete.
    get_du(du_idx).push_ul_pdu(
        test_helpers::generate_ue_context_release_complete(ue_ctx->cu_ue_id.value(), ue_ctx->du_ue_id.value()));
    return true;
  }

  unsigned du_idx    = 0;
  unsigned cu_up_idx = 0;

  gnb_du_ue_f1ap_id_t    du_ue_id      = gnb_du_ue_f1ap_id_t::min;
  rnti_t                 crnti         = to_rnti(0x4601);
  amf_ue_id_t            amf_ue_id     = amf_ue_id_t::min;
  gnb_cu_up_ue_e1ap_id_t cu_up_e1ap_id = gnb_cu_up_ue_e1ap_id_t::min;

  const ue_context* ue_ctx = nullptr;

  pdu_session_id_t psi  = uint_to_pdu_session_id(1);
  pdu_session_id_t psi2 = uint_to_pdu_session_id(2);
  qos_flow_id_t    qfi  = uint_to_qos_flow_id(1);
  qos_flow_id_t    qfi2 = uint_to_qos_flow_id(2);

  e1ap_message e1ap_pdu;
  f1ap_message f1ap_pdu;
  ngap_message ngap_pdu;
};

TEST_F(cu_cp_rrc_inactive_test, when_ue_level_inactivity_message_received_then_ue_is_set_to_rrc_inactive)
{
  // Inject Inactivity Notification and await Bearer Context Modification Request.
  ASSERT_TRUE(send_ue_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request());

  // Send Bearer Context Modification Response and await UE Context Release Command.
  ASSERT_TRUE(send_bearer_context_modification_response_and_await_ue_context_release_command());

  // Send F1AP UE Context Release Complete.
  ASSERT_TRUE(send_f1ap_ue_context_release_complete());
}

TEST_F(cu_cp_rrc_inactive_test,
       when_drb_level_inactivity_message_with_inactivity_for_all_drbs_received_then_ue_is_set_to_rrc_inactive)
{
  // Inject Inactivity Notification and await Bearer Context Modification Request.
  ASSERT_TRUE(send_drb_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request());

  // Send Bearer Context Modification Response and await UE Context Release Command.
  ASSERT_TRUE(send_bearer_context_modification_response_and_await_ue_context_release_command());

  // Send F1AP UE Context Release Complete.
  ASSERT_TRUE(send_f1ap_ue_context_release_complete());
}

TEST_F(cu_cp_rrc_inactive_test,
       when_pdu_session_level_inactivity_message_with_inactivity_for_all_drbs_received_then_ue_is_set_to_rrc_inactive)
{
  // Inject Inactivity Notification and await Bearer Context Modification Request.
  ASSERT_TRUE(
      send_pdu_session_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request());

  // Send Bearer Context Modification Response and await UE Context Release Command.
  ASSERT_TRUE(send_bearer_context_modification_response_and_await_ue_context_release_command());

  // Send F1AP UE Context Release Complete.
  ASSERT_TRUE(send_f1ap_ue_context_release_complete());
}
