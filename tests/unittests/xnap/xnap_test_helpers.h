/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "lib/xnap/xnap_impl.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/executors/manual_task_worker.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"
#include "ocudu/xnap/xnap_message.h"
#include "ocudu/xnap/xnap_message_notifier.h"
#include <gtest/gtest.h>

namespace ocudu::ocucp {

/// Dummy class to check for XN TX messages.
class dummy_xnap_message_notifier : public xnap_message_notifier
{
public:
  dummy_xnap_message_notifier(xnap_message& last_msg_) : last_msg(last_msg_) {}
  ~dummy_xnap_message_notifier() override = default;

  bool on_new_message(const xnap_message& msg) override
  {
    last_msg = msg;
    return true;
  }

  xnap_message& last_msg;
};

/// Reusable class that stores the messages sent over XNAP for test inspection.
class dummy_xnc_gateway : public xnc_connection_gateway
{
public:
  dummy_xnc_gateway() : logger(ocudulog::fetch_basic_logger("TEST")) {}

  std::unique_ptr<xnap_message_notifier> get_init_tx_notifier(transport_layer_address peer_addr) override
  {
    return std::make_unique<dummy_xnap_message_notifier>(last_tx_msg);
  }

  void attach_cu_cp(cu_cp_xnc_handler& xnc_handler_) override { logger.info("CU-CP attached to XN-C gateway"); }

  std::optional<uint16_t> get_listen_port() const override { return std::nullopt; }

  xnap_message get_last_tx_message() const { return last_tx_msg; }

private:
  xnap_message            last_tx_msg;
  ocudulog::basic_logger& logger;
};

/// Fixture class for XNAP Setup tests.
class xnap_test : public ::testing::Test
{
protected:
  void SetUp() override;

  void TearDown() override;

  void init_sctp_association()
  {
    xnap->set_tx_association_notifier(std::make_unique<dummy_xnap_message_notifier>(last_tx_msg));
  }

  ocudulog::basic_logger&    logger = ocudulog::fetch_basic_logger("TEST", false);
  manual_task_worker         ctrl_worker{128};
  dummy_xnc_gateway          xnc_gw;
  std::unique_ptr<xnap_impl> xnap = nullptr;

  gnb_id_t               local_gnb_id             = {0, 22};
  plmn_identity          local_plmn               = plmn_identity::test_value();
  tac_t                  local_tac                = {8};
  s_nssai_t              local_slice              = {};
  std::vector<s_nssai_t> local_slice_support_list = {local_slice};
  xnap_configuration     xnap_local_cfg           = {
      local_gnb_id,
      std::vector<supported_tracking_area>{{local_tac, std::vector<plmn_item>{{local_plmn, local_slice_support_list}}}},
      std::vector<guami_t>{{.plmn = local_plmn, .amf_set_id = 0, .amf_pointer = 0, .amf_region_id = 1}}};

  xnap_message get_last_message() { return last_tx_msg; }

private:
  xnap_message last_tx_msg;
};

} // namespace ocudu::ocucp
