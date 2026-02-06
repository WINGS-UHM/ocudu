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
#include "tests/test_doubles/ngap/ngap_test_message_validators.h"
#include "tests/unittests/ngap/ngap_test_messages.h"
#include "ocudu/asn1/ngap/ngap_ies.h"
#include "ocudu/asn1/ngap/ngap_pdu_contents.h"
#include "ocudu/ngap/ngap_message.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocucp;

class cu_cp_location_reporting_test : public cu_cp_test_environment, public ::testing::Test
{
public:
  cu_cp_location_reporting_test() : cu_cp_test_environment(cu_cp_test_env_params{})
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
  }

  [[nodiscard]] bool attach_ue()
  {
    if (!cu_cp_test_environment::attach_ue(
            du_idx, cu_up_idx, du_ue_id, crnti, amf_ue_id, cu_up_e1ap_id, psi, drb_id_t::drb1, qfi)) {
      return false;
    }
    ue_ctx = this->find_ue_context(du_idx, du_ue_id);
    return ue_ctx != nullptr;
  }

  unsigned du_idx    = 0;
  unsigned cu_up_idx = 0;

  gnb_du_ue_f1ap_id_t    du_ue_id      = gnb_du_ue_f1ap_id_t::min;
  rnti_t                 crnti         = to_rnti(0x4601);
  amf_ue_id_t            amf_ue_id     = amf_ue_id_t::min;
  gnb_cu_up_ue_e1ap_id_t cu_up_e1ap_id = gnb_cu_up_ue_e1ap_id_t::min;

  pdu_session_id_t psi = uint_to_pdu_session_id(1);
  qos_flow_id_t    qfi = uint_to_qos_flow_id(1);

  const ue_context* ue_ctx = nullptr;

  ngap_message ngap_pdu;
};

TEST_F(cu_cp_location_reporting_test,
       when_location_reporting_control_with_direct_type_is_received_then_location_report_is_sent_to_amf)
{
  // Attach UE.
  ASSERT_TRUE(attach_ue());

  // Drain any pending NGAP messages.
  while (get_amf().try_pop_rx_pdu(ngap_pdu)) {
  }

  // Inject Location Reporting Control message from AMF.
  get_amf().push_tx_pdu(
      generate_location_reporting_control_message(ue_ctx->amf_ue_id.value(), ue_ctx->ran_ue_id.value()));

  // Wait for the Location Report to be sent to the AMF.
  ASSERT_TRUE(this->wait_for_ngap_tx_pdu(ngap_pdu));

  // Verify it's a Location Report.
  ASSERT_EQ(ngap_pdu.pdu.type().value, asn1::ngap::ngap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(ngap_pdu.pdu.init_msg().value.type(),
            asn1::ngap::ngap_elem_procs_o::init_msg_c::types_opts::location_report);

  const auto& location_report = ngap_pdu.pdu.init_msg().value.location_report();

  // Verify UE IDs.
  ASSERT_EQ(location_report->amf_ue_ngap_id, amf_ue_id_to_uint(ue_ctx->amf_ue_id.value()));
  ASSERT_EQ(location_report->ran_ue_ngap_id, ran_ue_id_to_uint(ue_ctx->ran_ue_id.value()));

  // Verify location report request type.
  ASSERT_EQ(location_report->location_report_request_type.event_type, asn1::ngap::event_type_opts::options::direct);
  ASSERT_EQ(location_report->location_report_request_type.report_area.value,
            asn1::ngap::report_area_opts::options::cell);

  // Verify user location info is NR.
  ASSERT_EQ(location_report->user_location_info.type(),
            asn1::ngap::user_location_info_c::types_opts::options::user_location_info_nr);

  const auto& user_loc_info = location_report->user_location_info.user_location_info_nr();

  // Verify NR-CGI matches the serving cell configured in the DU (default served_cell_item_info).
  ASSERT_EQ(user_loc_info.nr_cgi.nr_cell_id.to_number(),
            nr_cell_identity::create(gnb_id_t{411, 22}, 0).value().value());
  ASSERT_EQ(user_loc_info.nr_cgi.plmn_id.to_number(), plmn_identity::test_value().to_bcd());

  // Verify TAI matches the serving cell TAC (default tac=7) and PLMN.
  ASSERT_EQ(user_loc_info.tai.plmn_id.to_number(), plmn_identity::test_value().to_bcd());
  ASSERT_EQ(user_loc_info.tai.tac.to_number(), 7);
}
