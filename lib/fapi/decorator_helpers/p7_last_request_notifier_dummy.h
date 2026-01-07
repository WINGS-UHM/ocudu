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

#include "ocudu/fapi/p7/p7_last_request_notifier.h"

namespace ocudu {
namespace fapi {

/// Dummy P7 last request notifier that will close the application if its methods are called.
class p7_last_request_notifier_dummy : public p7_last_request_notifier
{
public:
  // See interface for documentation.
  void on_last_message(slot_point slot) override;
};

} // namespace fapi
} // namespace ocudu
