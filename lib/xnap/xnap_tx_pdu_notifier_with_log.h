// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "log_helpers.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/xnap/xnap_message.h"
#include "ocudu/xnap/xnap_message_notifier.h"

namespace ocudu::ocucp {

class xnap_tx_pdu_notifier_with_logging final : public xnap_message_notifier
{
public:
  xnap_tx_pdu_notifier_with_logging(std::unique_ptr<xnap_message_notifier> xn_setup_request_notifier_) :
    xn_setup_request_notifier(std::move(xn_setup_request_notifier_)),
    logger(ocudulog::fetch_basic_logger("XNAP", false))
  {
  }

  ~xnap_tx_pdu_notifier_with_logging() override
  {
    if (decorated) {
      decorated.reset();
    }
  }

  void connect(std::unique_ptr<xnap_message_notifier> decorated_) { decorated = std::move(decorated_); }

  void disconnect()
  {
    if (is_connected()) {
      decorated.reset();
    }
  }

  bool is_connected() const { return decorated != nullptr; }

  /// This callback is invoked on an outgoing XN Setup request message. This message will initiate the SCTP association
  /// setup and therefore requires special handling.
  [[nodiscard]] bool on_xn_setup_request(const xnap_message& setup_request)
  {
    if (decorated != nullptr) {
      logger.warning("Requested to send XN setup request but a notifier with SCTP association is already connected");
      return false;
    }

    // Check if the message is an XN setup request, and if so, forward it to the XN setup request notifier.
    if (setup_request.pdu.type().value == asn1::xnap::xn_ap_pdu_c::types_opts::init_msg &&
        setup_request.pdu.init_msg().value.type().value ==
            asn1::xnap::xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request) {
      log_xnap_pdu(logger, logger.debug.enabled(), false, setup_request.pdu);
      return xn_setup_request_notifier->on_new_message(setup_request);
    }

    logger.warning("Requested to send XN setup request but the provided message is not an XN setup request");
    return false;
  }

  [[nodiscard]] bool on_new_message(const xnap_message& msg) override
  {
    log_xnap_pdu(logger, logger.debug.enabled(), false, msg.pdu);

    if (decorated == nullptr) {
      return false;
    }
    return decorated->on_new_message(msg);
  }

private:
  std::unique_ptr<xnap_message_notifier> xn_setup_request_notifier;
  ocudulog::basic_logger&                logger;
  std::unique_ptr<xnap_message_notifier> decorated;
};
} // namespace ocudu::ocucp
