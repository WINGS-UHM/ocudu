// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ue_context/xnap_ue_context.h"
#include "xnap_tx_pdu_notifier_with_log.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/xnap/xnap.h"
#include "ocudu/xnap/xnap_configuration.h"
#include "ocudu/xnap/xnap_message.h"

namespace ocudu::ocucp {

class xnap_impl final : public xnap_interface
{
public:
  xnap_impl(const xnap_configuration&              xnap_cfg_,
            xnap_cu_cp_notifier&                   cu_cp_notifier_,
            std::unique_ptr<xnap_message_notifier> init_tx_notifier_,
            timer_manager&                         timers_,
            task_executor&                         ctrl_exec_);
  ~xnap_impl() override = default;

  // XNAP message handling.
  void handle_message(const xnap_message& msg) override;

  async_task<void> stop() override;

  // XNAP connection manager functions.
  async_task<bool> handle_xn_setup_request_required() override;
  void             set_tx_association_notifier(std::unique_ptr<xnap_message_notifier> tx_notifier_) override
  {
    tx_notifier.connect(std::move(tx_notifier_));
  }
  async_task<xnap_handover_preparation_response>
  handle_handover_request_required(const xnap_handover_request& request) override;
  async_task<expected<cu_cp_status_transfer>> handle_sn_status_transfer_required(ue_index_t ue_index) override;

private:
  /// \brief Notify about the reception of an initiating message.
  /// \param[in] msg The received initiating message.
  void handle_initiating_message(const asn1::xnap::init_msg_s& msg);

  /// \brief Notify about the reception of a Handover Request message.
  /// \param[in] msg The received handover request message.
  void handle_handover_request(const asn1::xnap::ho_request_s& msg);

  /// \brief Notify about the reception of a Handover Cancel message.
  /// \param[in] msg The received handover cancel message.
  void handle_handover_cancel(const asn1::xnap::ho_cancel_s& msg);

  /// \brief Notify about the reception of a SN Status Transfer message.
  /// \param[in] msg The received SN Status Transfer message.
  void handle_sn_status_transfer(const asn1::xnap::sn_status_transfer_s& msg);

  /// \brief Notify about the reception of a successful outcome message.
  /// \param[in] outcome The successful outcome message.
  void handle_successful_outcome(const asn1::xnap::successful_outcome_s& outcome);

  /// \brief Notify about the reception of an unsuccessful outcome message.
  /// \param[in] outcome The unsuccessful outcome message.
  void handle_unsuccessful_outcome(const asn1::xnap::unsuccessful_outcome_s& outcome);

  void handle_xn_setup_request(const asn1::xnap::xn_setup_request_s& msg);

  ocudulog::basic_logger& logger;

  /// Repository of UE Contexts.
  xnap_ue_context_list ue_ctxt_list;

  xnap_configuration   xnap_cfg;
  xnap_cu_cp_notifier& cu_cp_notifier;
  timer_manager&       timers;
  task_executor&       ctrl_exec;

  xnap_tx_pdu_notifier_with_logging tx_notifier;

  /// XN Setup Response/Failure Event Source.
  protocol_transaction_event_source<asn1::xnap::xn_setup_resp_s, asn1::xnap::xn_setup_fail_s> xn_setup_outcome;
};

} // namespace ocudu::ocucp
