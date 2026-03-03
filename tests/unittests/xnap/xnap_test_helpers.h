// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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

class dummy_xnap_cu_cp_notifier : public xnap_cu_cp_notifier
{
public:
  dummy_xnap_cu_cp_notifier() = default;

  byte_buffer on_handover_preparation_message_required(ue_index_t ue_index) override
  {
    logger.info("Handover preparation message requested for UE index {}", ue_index);
    return byte_buffer{};
  }

  async_task<bool> on_new_rrc_handover_command(ue_index_t ue_index, byte_buffer command) override
  {
    logger.info("Received a new RRC Handover Command for UE index {}", ue_index);
    last_handover_command = std::move(command);
    return launch_async([](coro_context<async_task<bool>>& ctx) mutable {
      CORO_BEGIN(ctx);
      CORO_RETURN(true);
    });
  }

  byte_buffer last_handover_command;

private:
  ocudulog::basic_logger& logger = ocudulog::fetch_basic_logger("TEST");
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

  /// \brief Manually tick timers.
  template <typename T>
  void tick(async_task<T>& task, std::chrono::milliseconds duration)
  {
    for (unsigned msec_elapsed = 0; msec_elapsed < duration.count(); ++msec_elapsed) {
      ASSERT_FALSE(task.ready());
      timers.tick();
      ctrl_worker.run_pending_tasks();
    }
  }

  ocudulog::basic_logger&    logger = ocudulog::fetch_basic_logger("TEST", false);
  timer_manager              timers;
  manual_task_worker         ctrl_worker{128};
  dummy_xnc_gateway          xnc_gw;
  dummy_xnap_cu_cp_notifier  cu_cp_notifier;
  std::unique_ptr<xnap_impl> xnap = nullptr;

  /// Local configuration.
  gnb_id_t               local_gnb_id             = {0, 22};
  plmn_identity          local_plmn               = plmn_identity::test_value();
  tac_t                  local_tac                = {8};
  s_nssai_t              local_slice              = {};
  std::vector<s_nssai_t> local_slice_support_list = {local_slice};
  xnap_configuration     xnap_local_cfg           = {
      local_gnb_id,
      std::vector<supported_tracking_area>{{local_tac, std::vector<plmn_item>{{local_plmn, local_slice_support_list}}}},
      std::vector<guami_t>{{.plmn = local_plmn, .amf_set_id = 0, .amf_pointer = 0, .amf_region_id = 1}}};

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

  xnap_message get_last_message() { return last_tx_msg; }

private:
  xnap_message last_tx_msg;
};

} // namespace ocudu::ocucp
