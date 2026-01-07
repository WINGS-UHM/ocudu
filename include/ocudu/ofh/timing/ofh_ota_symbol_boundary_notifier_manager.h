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

#include "ocudu/adt/span.h"
#include "ocudu/ofh/timing/ofh_ota_symbol_boundary_notifier.h"

namespace ocudu {
namespace ofh {

/// \brief OTA symbol boundary notifier manager.
///
/// Interface to subscribe OTA symbol boundary notifiers.
class ota_symbol_boundary_notifier_manager
{
public:
  /// Default destructor.
  virtual ~ota_symbol_boundary_notifier_manager() = default;

  /// Subscribes the given notifiers to listen to OTA symbol boundary events.
  virtual void subscribe(span<ota_symbol_boundary_notifier*> notifiers) = 0;
};

} // namespace ofh
} // namespace ocudu
