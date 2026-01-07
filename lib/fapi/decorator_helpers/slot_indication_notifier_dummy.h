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

#include "ocudu/fapi/p7/p7_slot_indication_notifier.h"

namespace ocudu {
namespace fapi {

/// Dummy FAPI slot indication notifier that will close the application if its methods are called.
class slot_indication_notifier_dummy : public p7_slot_indication_notifier
{
public:
  // See interface for documentation.
  void on_slot_indication(const slot_indication& msg) override;
};

} // namespace fapi
} // namespace ocudu
