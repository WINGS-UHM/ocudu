// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "xnap_message_notifier.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu::ocucp {

struct xnap_message;

/// This interface is used to push XNAP messages to the XNAP interface.
class xnap_message_handler
{
public:
  virtual ~xnap_message_handler() = default;

  /// Handle the incoming XNAP message.
  virtual void handle_message(const xnap_message& msg) = 0;
};

/// XNAP interface for CU-CP to initiate the XN interface.
class xnap_connection_manager
{
public:
  virtual ~xnap_connection_manager() = default;

  /// Trigger the initiation of the XN setup procedure.
  virtual async_task<void> handle_xn_setup_request_required() = 0;

  /// Provide the SCTP association notifier after the SCTP association establishment.
  virtual void set_tx_association_notifier(std::unique_ptr<xnap_message_notifier> tx_notifier_) = 0;
};

/// XNAP notifier to the CU-CP.
class xnap_cu_cp_notifier
{
public:
  virtual ~xnap_cu_cp_notifier() = default;

  // TODO.
};

/// Combined entry point for the XNAP object.
class xnap_interface : public xnap_message_handler, public xnap_connection_manager
{
public:
  virtual ~xnap_interface() = default;
};

} // namespace ocudu::ocucp
