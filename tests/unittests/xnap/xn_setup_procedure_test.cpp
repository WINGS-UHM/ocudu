/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "lib/xnap/xnap_impl.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/support/executors/manual_task_worker.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace ocudu::ocucp;

static xnap_message generate_xn_setup_request(const xnap_configuration& xnap_cfg);

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

    xnap = std::make_unique<xnap_impl>(xnap_cfg, ctrl_worker);
  }

  void TearDown() override
  {
    // Flush logger after each test.
    ocudulog::flush();
  }

  ocudulog::basic_logger&    logger = ocudulog::fetch_basic_logger("TEST", false);
  manual_task_worker         ctrl_worker{128};
  std::unique_ptr<xnap_impl> xnap = nullptr;
  xnap_configuration         xnap_cfg{gnb_id_t{0, 22}};

  /// Peer configuration.
  gnb_id_t               peer_gnb_id        = {1, 22};
  plmn_identity          peer_plmn          = plmn_identity::test_value();
  tac_t                  peer_tac           = {7};
  s_nssai_t              peer_slice         = {};
  std::vector<s_nssai_t> slice_support_list = {peer_slice};

  xnap_configuration xnap_peer_cfg = {
      peer_gnb_id,
      std::vector<supported_tracking_area>{{peer_tac, std::vector<plmn_item>{{peer_plmn, slice_support_list}}}}};
};

TEST_F(xn_setup_procedure_test, when_correct_setup_received_from_peer_setup_complete_is_sent)
{
  xnap_message xn_setup_req = generate_xn_setup_request(xnap_peer_cfg);
  xnap->handle_message(xn_setup_req);
}

/// Helper functions.
static xnap_message generate_xn_setup_request(const xnap_configuration& xnap_cfg)
{
  xnap_message xn_setup_req;
  xn_setup_req.pdu.set_init_msg();
  xn_setup_req.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_XN_SETUP);

  asn1::xnap::xn_setup_request_s& asn1_ies = xn_setup_req.pdu.init_msg().value.xn_setup_request();

  // Fill global RAN node id.
  auto& global_gnb = asn1_ies->global_ng_ran_node_id.set_gnb();
  global_gnb.gnb_id.set_gnb_id();
  global_gnb.gnb_id.gnb_id().from_number(xnap_cfg.gnb_id.id, xnap_cfg.gnb_id.bit_length);

  // Fill TAI support list.
  asn1_ies->tai_support_list.resize(xnap_cfg.tai_support_list.size());
  for (unsigned i = 0; i < xnap_cfg.tai_support_list.size(); i++) {
    const auto& tai_support_item      = xnap_cfg.tai_support_list[i];
    auto&       asn1_tai_support_item = asn1_ies->tai_support_list[i];

    // Fill TAC.
    asn1_tai_support_item.tac.from_number(tai_support_item.tac);

    // Fill broadcast PLMN list.
    asn1_tai_support_item.broadcast_plmns.resize(tai_support_item.plmn_list.size());
    for (unsigned j = 0; j < xnap_cfg.tai_support_list.size(); j++) {
      const auto& plmn_item           = tai_support_item.plmn_list[j];
      auto&       asn1_broadcast_plmn = asn1_tai_support_item.broadcast_plmns[j];

      // Fill PLMN id.
      asn1_broadcast_plmn.plmn_id.from_number(plmn_item.plmn_id.to_bcd());

      // Fill TAI slice support list.
      asn1_broadcast_plmn.tai_slice_support_list.resize(plmn_item.slice_support_list.size());
      for (unsigned k = 0; k < plmn_item.slice_support_list.size(); k++) {
        // Fill S-NSSAI.
        const auto& snssai_item    = plmn_item.slice_support_list[k];
        auto&       asn1_tai_slice = asn1_broadcast_plmn.tai_slice_support_list[k];
        asn1_tai_slice.sst.from_number(snssai_item.sst.value());

        if (snssai_item.sd.is_set()) {
          asn1_tai_slice.sd_present = true;
          asn1_tai_slice.sd.from_number(snssai_item.sd.value());
        }
      }
    }
  }

  // Fill AMF region information.
  asn1_ies->amf_region_info.resize(1);

  return xn_setup_req;
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
