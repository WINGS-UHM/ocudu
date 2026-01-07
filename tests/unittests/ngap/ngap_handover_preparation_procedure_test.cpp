/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "tests/unittests/ngap/ngap_test_helpers.h"
#include "tests/unittests/ngap/ngap_test_messages.h"
#include "ocudu/ngap/ngap_handover.h"
#include "ocudu/ran/cu_types.h"
#include "ocudu/support/async/async_test_utils.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocucp;

/// Test successful handover preparation procedure.
TEST_F(ngap_test, when_source_gnb_handover_preparation_triggered_then_ho_command_received)
{
  // Setup UE context
  ue_index_t ue_index = create_ue();
  run_dl_nas_transport(ue_index); // needed to allocate AMF UE id.

  // Manually add existing PDU sessions to UP manager.
  add_pdu_session_to_up_manager(ue_index, uint_to_pdu_session_id(1), uint_to_drb_id(1), uint_to_qos_flow_id(0));

  auto& ue = test_ues.at(ue_index);
  ue.rrc_ue_handler.set_ho_preparation_message({});

  ngap_handover_preparation_request request =
      generate_handover_preparation_request(ue_index,
                                            ue_mng.find_ue(ue_index)->get_up_resource_manager().get_pdu_sessions_map(),
                                            nr_cell_identity::create({1, 22}, 1).value(),
                                            22);

  // Action 1: Launch HO preparation procedure
  test_logger.info("Launch source NGAP handover preparation procedure");
  async_task<ngap_handover_preparation_response>         t = ngap->handle_handover_preparation_request(request);
  lazy_task_launcher<ngap_handover_preparation_response> t_launcher(t);

  // Status: AMF received Handover Required.
  ASSERT_EQ(n2_gw.last_ngap_msgs.back().pdu.type().value, asn1::ngap::ngap_pdu_c::types_opts::init_msg);
  ASSERT_EQ(n2_gw.last_ngap_msgs.back().pdu.init_msg().value.type().value,
            asn1::ngap::ngap_elem_procs_o::init_msg_c::types_opts::ho_required);

  ASSERT_FALSE(t.ready());

  // Inject Handover Command.
  ngap_message ho_cmd = generate_valid_handover_command(ue.amf_ue_id.value(), ue.ran_ue_id.value());
  ngap->handle_message(ho_cmd);

  // Procedure should have succeeded.
  ASSERT_TRUE(t.ready());
  ASSERT_TRUE(t.get().success);
}
