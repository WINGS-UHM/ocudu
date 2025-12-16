/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
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
#include "tests/test_doubles/rrc/rrc_test_messages.h"
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
    report_fatal_error_if_not(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                              "there are still E1AP messages to pop from CU-UP");
    report_fatal_error_if_not(not this->get_du(du_idx).try_pop_dl_pdu(f1ap_pdu),
                              "there are still F1AP DL messages to pop from DU");

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
    report_fatal_error_if_not(not this->get_cu_up(cu_up_idx).try_pop_rx_pdu(e1ap_pdu),
                              "there are still E1AP messages to pop from CU-UP");
    report_fatal_error_if_not(not this->get_du(du_idx).try_pop_dl_pdu(f1ap_pdu),
                              "there are still F1AP DL messages to pop from DU");

    // Inject F1AP UE Context Release Complete.
    get_du(du_idx).push_ul_pdu(
        test_helpers::generate_ue_context_release_complete(ue_ctx->cu_ue_id.value(), ue_ctx->du_ue_id.value()));
    return true;
  }

  [[nodiscard]] bool send_init_ul_rrc_message_transfer_and_await_ue_context_setup_request()
  {
    // Inject Initial UL RRC message and await UE context setup request.
    f1ap_message init_ul_rrc_msg = test_helpers::generate_init_ul_rrc_message_transfer(
        du_ue_id_2,
        crnti_2,
        plmn_identity::test_value(),
        {},
        test_helpers::pack_ul_ccch_msg(test_helpers::create_rrc_resume_request(0x36000)));
    test_logger.info("c-rnti={} du_ue={}: Injecting Initial UL RRC message", crnti_2, fmt::underlying(du_ue_id_2));
    get_du(du_idx).push_ul_pdu(init_ul_rrc_msg);
    report_fatal_error_if_not(this->wait_for_f1ap_tx_pdu(du_idx, f1ap_pdu),
                              "Failed to receive UE Context Setup Request");
    report_fatal_error_if_not(test_helpers::is_valid_ue_context_setup_request(f1ap_pdu),
                              "Invalid UE Context Setup Request");

    return true;
  }

  [[nodiscard]] bool send_ue_context_setup_response_and_await_bearer_context_modification_request()
  {
    // Inject UE Context Setup Response and wait for Bearer Context Modification Request.
    get_du(du_idx).push_ul_pdu(test_helpers::generate_ue_context_setup_response(
        ue_ctx->cu_ue_id.value(),
        du_ue_id_2,
        crnti_2,
        make_byte_buffer(
            "5c04c00604b0701f00811a0f020001273b8c02692f30004d25e24040008c8040a26418d6d8d76006e08040000101000083446a48d8"
            "02692f1200000464e35b63224f8060664abff0124e9106e28dc61b8e372c6fbf56c70eb00442c0680182c4601c020521004930b2a0"
            "003fe00000000060dc2108000780594008300000020698101450a000e3890000246aac90838002081840a1839389142c60d1c3c811"
            "00000850000800b50001000850101800b50102000850202800b50203000850303800b503040c885010480504014014120580505018"
            "01416068050601c0141a0780507020314100880905204963028711d159e26f2681d2083c5df81821c00000038ffd294a5294f28160"
            "00021976000000000000000000108ad5450047001800082000e21009c400e0202108001c420138401c080441000388402708038180"
            "842000710804e18070401104000e21009c300080000008218081018201c1a0001c71000000080100020180020240088029800008c4"
            "0089c7001800")
            .value(),
        {drb_id_t::drb1}));

    report_fatal_error_if_not(this->wait_for_e1ap_tx_pdu(cu_up_idx, e1ap_pdu),
                              "Failed to receive Bearer Context Modification Request");
    report_fatal_error_if_not(test_helpers::is_valid_bearer_context_modification_request(e1ap_pdu),
                              "Invalid Bearer Context Modification Request");
    return true;
  }

  [[nodiscard]] bool send_bearer_context_modification_response_and_await_dl_rrc_message_transfer()
  {
    // Inject Bearer Context Modification Response and wait for DL RRC Message (containing RRC Resume).
    get_cu_up(cu_up_idx).push_tx_pdu(
        generate_bearer_context_modification_response(ue_ctx->cu_cp_e1ap_id.value(), cu_up_e1ap_id));
    report_fatal_error_if_not(this->wait_for_f1ap_tx_pdu(du_idx, f1ap_pdu),
                              "Failed to receive F1AP DL RRC Message (containing RRC Resume)");
    report_fatal_error_if_not(test_helpers::is_valid_dl_rrc_message_transfer(f1ap_pdu),
                              "Invalid DL RRC Message Transfer");
    {
      const byte_buffer& rrc_container = test_helpers::get_rrc_container(f1ap_pdu);

      report_fatal_error_if_not(test_helpers::is_valid_rrc_resume(test_helpers::extract_dl_dcch_msg(rrc_container)),
                                "Invalid RRC Resume");
    }
    return true;
  }

  [[nodiscard]] bool send_rrc_resume_complete()
  {
    // Inject UL RRC Message (containing RRC Resume Complete).
    f1ap_message init_ul_rrc_msg = test_helpers::generate_ul_rrc_message_transfer(
        du_ue_id_2, ue_ctx->cu_ue_id.value(), srb_id_t::srb1, make_byte_buffer("000020400033b01cab").value());
    get_du(du_idx).push_ul_pdu(init_ul_rrc_msg);
    return true;
  }

  unsigned du_idx    = 0;
  unsigned cu_up_idx = 0;

  gnb_du_ue_f1ap_id_t    du_ue_id      = gnb_du_ue_f1ap_id_t::min;
  rnti_t                 crnti         = to_rnti(0x4601);
  amf_ue_id_t            amf_ue_id     = amf_ue_id_t::min;
  gnb_cu_up_ue_e1ap_id_t cu_up_e1ap_id = gnb_cu_up_ue_e1ap_id_t::min;

  gnb_du_ue_f1ap_id_t du_ue_id_2 = int_to_gnb_du_ue_f1ap_id(1);
  rnti_t              crnti_2    = to_rnti(0x4602);

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

  // Check metrics for RRC inactive transition.
  auto report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus[0].rrc_metrics.mean_nof_inactive_rrc_connections, 1);
  ASSERT_EQ(report.dus[0].rrc_metrics.max_nof_inactive_rrc_connections, 1);
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

  // Check metrics for RRC inactive transition.
  auto report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus[0].rrc_metrics.mean_nof_inactive_rrc_connections, 1);
  ASSERT_EQ(report.dus[0].rrc_metrics.max_nof_inactive_rrc_connections, 1);
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

  // Check metrics for RRC inactive transition.
  auto report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus[0].rrc_metrics.mean_nof_inactive_rrc_connections, 1);
  ASSERT_EQ(report.dus[0].rrc_metrics.max_nof_inactive_rrc_connections, 1);
}

TEST_F(cu_cp_rrc_inactive_test, when_rrc_resume_request_is_received_then_existing_ue_is_found_and_resumed)
{
  // Check metrics for active RRC UE.
  auto report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus[0].rrc_metrics.mean_nof_rrc_connections, 1);
  ASSERT_EQ(report.dus[0].rrc_metrics.max_nof_rrc_connections, 1);

  // Inject Inactivity Notification and await Bearer Context Modification Request.
  ASSERT_TRUE(send_ue_level_bearer_context_inactivity_notification_and_await_bearer_context_modification_request());

  // Send Bearer Context Modification Response and await UE Context Release Command.
  ASSERT_TRUE(send_bearer_context_modification_response_and_await_ue_context_release_command());

  // Send F1AP UE Context Release Complete.
  ASSERT_TRUE(send_f1ap_ue_context_release_complete());

  // Check metrics for RRC inactive transition.
  report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus[0].rrc_metrics.mean_nof_inactive_rrc_connections, 1);
  ASSERT_EQ(report.dus[0].rrc_metrics.max_nof_inactive_rrc_connections, 1);

  // Send Initial UL RRC Message containing RRC Resume Request.
  ASSERT_TRUE(send_init_ul_rrc_message_transfer_and_await_ue_context_setup_request());

  // Check metrics for attempted RRC resume.
  report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(report.dus[0].rrc_metrics.attempted_rrc_connection_resumes.get_count(establishment_resume_cause_t::mo_data),
            1);

  // Send UE Context Setup Response and await Bearer Context Modification Request.
  ASSERT_TRUE(send_ue_context_setup_response_and_await_bearer_context_modification_request());

  // Send Bearer Context Modification Response and await DL RRC Message Transfer.
  ASSERT_TRUE(send_bearer_context_modification_response_and_await_dl_rrc_message_transfer());

  // Send RRC Resume Complete.
  ASSERT_TRUE(send_rrc_resume_complete());

  // Check metrics for successful RRC resume.
  report = this->get_cu_cp().get_metrics_handler().request_metrics_report();
  ASSERT_EQ(
      report.dus[0].rrc_metrics.successful_rrc_connection_resumes.get_count(establishment_resume_cause_t::mo_data), 1);
}
