// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "procedures/xnap_transaction_manager.h"
#include "xnap_tx_pdu_notifier_with_log.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_configuration.h"
#include "ocudu/xnap/xnap_message.h"

namespace ocudu::ocucp {

class xnap_impl final : public xnap_interface
{
public:
  xnap_impl(const xnap_configuration&              xnap_cfg_,
            std::unique_ptr<xnap_message_notifier> init_tx_notifier_,
            timer_manager&                         timers_,
            task_executor&                         ctrl_exec_);
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
  /// \brief Notify about the reception of an initiating message.
  /// \param[in] msg The received initiating message.
  void handle_initiating_message(const asn1::xnap::init_msg_s& msg);

  /// \brief Notify about the reception of a successful outcome message.
  /// \param[in] outcome The successful outcome message.
  void handle_successful_outcome(const asn1::xnap::successful_outcome_s& outcome);

  /// \brief Notify about the reception of an unsuccessful outcome message.
  /// \param[in] outcome The unsuccessful outcome message.
  void handle_unsuccessful_outcome(const asn1::xnap::unsuccessful_outcome_s& outcome);

  void handle_xn_setup_request(const asn1::xnap::xn_setup_request_s& msg);

  ocudulog::basic_logger& logger;

  xnap_configuration xnap_cfg;
  timer_manager&     timers;
  task_executor&     ctrl_exec;

  xnap_transaction_manager ev_mng;

  xnap_tx_pdu_notifier_with_logging tx_notifier;
};

} // namespace ocudu::ocucp
