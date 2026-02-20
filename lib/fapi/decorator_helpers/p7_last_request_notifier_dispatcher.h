// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/p7/p7_last_request_notifier.h"

namespace ocudu {
namespace fapi {

/// Slot P7 last request notifier dispatcher that forwards last request notifications to the configured notifier.
class p7_last_request_notifier_dispatcher : public p7_last_request_notifier
{
  p7_last_request_notifier* p7_notifier = nullptr;

public:
  p7_last_request_notifier_dispatcher();

  // See interface for documentation.
  void on_last_message(slot_point slot) override;

  /// Sets the P7 last requests notifier to the given one.
  void set_p7_last_request_notifier(p7_last_request_notifier& last_msg_notifier);
};

} // namespace fapi
} // namespace ocudu
