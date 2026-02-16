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

#include "xnap_tx_pdu_notifier_with_log.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_configuration.h"
#include "ocudu/xnap/xnap_message.h"

namespace ocudu::ocucp {

class xnap_impl final : public xnap_interface
{
public:
  xnap_impl(const xnap_configuration& xnap_cfg_, xnc_connection_gateway& xnc_gw_, task_executor& ctrl_exec_);
  ~xnap_impl() override = default;

  // XNAP message handling.
  void handle_message(const xnap_message& msg) override;

  // XNAP connection manager functions.
  async_task<void> handle_xn_setup_request_required() override;
  void             set_tx_association_notifier(std::unique_ptr<xnap_message_notifier> tx_notifier_) override
  {
    tx_notifier.connect(std::move(tx_notifier_));
  }

private:
  /// Message handling.
  void handle_initiating_message(const asn1::xnap::init_msg_s& msg);
  void handle_successful_outcome(const asn1::xnap::successful_outcome_s& msg);
  void handle_unsuccessful_outcome(const asn1::xnap::unsuccessful_outcome_s& msg);

  void handle_xn_setup_request(const asn1::xnap::xn_setup_request_s& msg);

  ocudulog::basic_logger& logger;

  xnap_configuration      xnap_cfg;
  xnc_connection_gateway& xnc_gw;
  task_executor&          ctrl_exec;

  xnap_tx_pdu_notifier_with_logging tx_notifier;
};

} // namespace ocudu::ocucp
