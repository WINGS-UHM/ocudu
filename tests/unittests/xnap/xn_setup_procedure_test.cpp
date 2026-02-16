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
#include "xnap_test_helpers.h"
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
  xnap_message xn_setup_req = generate_asn1_xn_setup_request(xnap_peer_cfg);
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
