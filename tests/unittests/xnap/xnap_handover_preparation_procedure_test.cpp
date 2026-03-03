// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "tests/unittests/rrc/rrc_ue_test_helpers.h"
#include "tests/unittests/xnap/xnap_test_messages.h"
#include "xnap_test_helpers.h"
#include "ocudu/security/security.h"
#include "ocudu/support/async/async_test_utils.h"
#include "ocudu/xnap/xnap_handover.h"
#include <chrono>
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocucp;

/// Fixture class for XNAP handover preparation procedure tests.
class xnap_handover_preparation_procedure_test : public xnap_test
{
public:
  xnap_handover_preparation_procedure_test()           = default;
  ~xnap_handover_preparation_procedure_test() override = default;

  static std::map<pdu_session_id_t, up_pdu_session_context> generate_pdu_sessions()
  {
    // Generate PDU sessions map for the UE.
    up_pdu_session_context pdu_session_context{pdu_session_id_t::min, pdu_session_type_t::ipv4};

    qos_characteristics           qos_desc(non_dyn_5qi_descriptor{});
    qos_flow_level_qos_parameters qos_params{.qos_desc             = qos_desc,
                                             .alloc_retention_prio = alloc_and_retention_priority{}};

    pdu_session_context.drbs.emplace(
        drb_id_t::drb1,
        up_drb_context{.drb_id                          = drb_id_t::drb1,
                       .pdu_session_id                  = pdu_session_id_t::min,
                       .s_nssai                         = s_nssai_t{slice_service_type{1}, slice_differentiator{}},
                       .default_drb                     = false,
                       .rlc_mod                         = rlc_mode::am,
                       .qos_params                      = qos_params,
                       .qos_flows                       = {{qos_flow_id_t::min,
                                                            up_qos_flow_context{.qfi = qos_flow_id_t::min, .qos_params = qos_params}}},
                       .ul_up_tnl_info_to_be_setup_list = {
                           up_transport_layer_info{transport_layer_address::create_from_string("127.0.0.1"),
                                                   int_to_gtpu_teid(12345)},
                       }});

    std::map<pdu_session_id_t, up_pdu_session_context> pdu_sessions;
    pdu_sessions.emplace(pdu_session_id_t::min, pdu_session_context);

    return pdu_sessions;
  }

  static xnap_handover_preparation_request
  generate_handover_preparation_request(ue_index_t                                          ue_index,
                                        security::security_context&                         sec_ctxt,
                                        std::map<pdu_session_id_t, up_pdu_session_context>& pdu_sessions)
  {
    xnap_handover_preparation_request request = {
        .ue_index = ue_index,
        .nr_cgi   = nr_cell_global_id_t{plmn_identity::test_value(), nr_cell_identity::create({1, 22}, 1).value()},
        .guami    = guami_t{.plmn = plmn_identity::test_value(), .amf_set_id = 1, .amf_pointer = 1, .amf_region_id = 1},
        .amf_ue_id                     = amf_ue_id_to_uint(amf_ue_id_t::min),
        .amf_addr                      = transport_layer_address::create_from_string("127.0.0.1"),
        .sec_ctxt                      = sec_ctxt,
        .aggregate_maximum_bit_rate_dl = 0,
        .aggregate_maximum_bit_rate_ul = 0,
        .pdu_sessions                  = pdu_sessions};

    return request;
  }
};

/// Test unsuccessful handover preparation procedure.
TEST_F(xnap_handover_preparation_procedure_test, when_handover_preparation_failure_received_then_procedure_fails)
{
  // Run XN setup..
  run_xn_setup(xnap_peer_cfg);

  // Create UE context.
  ue_index_t ue_index = create_ue();

  // Generate Security context for the UE.
  security::security_context sec_ctxt = generate_security_context(ue_mng.find_ue(ue_index)->get_security_manager());
  // Generate PDU sessions map for the UE.
  std::map<pdu_session_id_t, up_pdu_session_context> pdu_sessions = generate_pdu_sessions();

  xnap_handover_preparation_request request = generate_handover_preparation_request(ue_index, sec_ctxt, pdu_sessions);

  // Action 1: Launch HO preparation procedure
  logger.info("Launch source XNAP handover preparation procedure");
  async_task<xnap_handover_preparation_response>         t = xnap->handle_handover_preparation_request(request);
  lazy_task_launcher<xnap_handover_preparation_response> t_launcher(t);

  // Status: XN-C peer received Handover Required.
  ASSERT_EQ(get_last_message().pdu.type().value, asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(get_last_message().pdu.init_msg().value.type().value,
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::ho_request);

  ASSERT_FALSE(t.ready());

  // Inject Handover Preparation Failure.
  xnap_message ho_prep_fail = generate_handover_preparation_failure(xnap_ue_id_t::min);
  xnap->handle_message(ho_prep_fail);

  // Procedure should have failed.
  ASSERT_TRUE(t.ready());
  ASSERT_FALSE(t.get().success);
}

/// Test unsuccessful handover preparation procedure.
TEST_F(xnap_handover_preparation_procedure_test, when_handover_preparation_times_out_then_handover_cancel_is_sent)
{
  // Run XN setup..
  run_xn_setup(xnap_peer_cfg);

  // Create UE context.
  ue_index_t ue_index = create_ue();

  // Generate Security context for the UE.
  security::security_context sec_ctxt = generate_security_context(ue_mng.find_ue(ue_index)->get_security_manager());
  // Generate PDU sessions map for the UE.
  std::map<pdu_session_id_t, up_pdu_session_context> pdu_sessions = generate_pdu_sessions();

  xnap_handover_preparation_request request = generate_handover_preparation_request(ue_index, sec_ctxt, pdu_sessions);

  // Action 1: Launch HO preparation procedure
  logger.info("Launch source XNAP handover preparation procedure");
  async_task<xnap_handover_preparation_response>         t = xnap->handle_handover_preparation_request(request);
  lazy_task_launcher<xnap_handover_preparation_response> t_launcher(t);

  // Status: XN-C peer received Handover Required.
  ASSERT_EQ(get_last_message().pdu.type().value, asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(get_last_message().pdu.init_msg().value.type().value,
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::ho_request);

  ASSERT_FALSE(t.ready());

  // Status: Fail Handover Preparation procedure (XN-C peer doesn't respond).
  ASSERT_TRUE(this->tick(t, std::chrono::milliseconds{1000}));

  // Status: XN-C peer received Handover Cancel.
  ASSERT_EQ(get_last_message().pdu.type().value, asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(get_last_message().pdu.init_msg().value.type().value,
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::ho_cancel);

  // Procedure should have failed.
  ASSERT_TRUE(t.ready());
  ASSERT_FALSE(t.get().success);
}

/// Test successful handover preparation procedure.
TEST_F(xnap_handover_preparation_procedure_test, when_handover_request_ack_received_then_procedure_succeeds)
{
  // Run XN setup..
  run_xn_setup(xnap_peer_cfg);

  // Create UE context.
  ue_index_t ue_index = create_ue();

  // Generate Security context for the UE.
  security::security_context sec_ctxt = generate_security_context(ue_mng.find_ue(ue_index)->get_security_manager());
  // Generate PDU sessions map for the UE.
  std::map<pdu_session_id_t, up_pdu_session_context> pdu_sessions = generate_pdu_sessions();

  xnap_handover_preparation_request request = generate_handover_preparation_request(ue_index, sec_ctxt, pdu_sessions);

  // Action 1: Launch HO preparation procedure
  logger.info("Launch source XNAP handover preparation procedure");
  async_task<xnap_handover_preparation_response>         t = xnap->handle_handover_preparation_request(request);
  lazy_task_launcher<xnap_handover_preparation_response> t_launcher(t);

  // Status: XN-C peer received Handover Required.
  ASSERT_EQ(get_last_message().pdu.type().value, asn1::xnap::xn_ap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(get_last_message().pdu.init_msg().value.type().value,
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::ho_request);

  ASSERT_FALSE(t.ready());

  // Inject Handover Request Acknowledge.
  xnap_message ho_request_ack = generate_handover_request_ack(xnap_ue_id_t::min, xnap_ue_id_t::min);
  xnap->handle_message(ho_request_ack);

  // Procedure should have succeeded.
  ASSERT_TRUE(t.ready());
  ASSERT_TRUE(t.get().success);
}
