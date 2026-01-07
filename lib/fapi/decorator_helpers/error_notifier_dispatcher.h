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

#include "ocudu/fapi/common/error_indication_notifier.h"

namespace ocudu {
namespace fapi {

/// Error message notifier dispatcher that forwards error messages to the configured notifier.
class error_notifier_dispatcher : public error_indication_notifier
{
  error_indication_notifier* notifier = nullptr;

public:
  error_notifier_dispatcher();

  // See interface for documentation.
  void on_error_indication(const error_indication& msg) override;

  /// Sets the error indication notifier to the given one.
  void set_error_indication_notifier(error_indication_notifier& error_notifier);
};

} // namespace fapi
} // namespace ocudu
