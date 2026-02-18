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

namespace ocudu::ocucp {

struct xnap_message;

/// Notifier interface used to notify outgoing XNAP messages.
class xnap_message_notifier
{
public:
  virtual ~xnap_message_notifier() = default;

  /// This callback is invoked on each outgoing XNAP message.
  virtual bool on_new_message(const xnap_message& msg) = 0;
};

} // namespace ocudu::ocucp
