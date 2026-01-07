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
#include "ocudu/ocudulog/ocudulog.h"

namespace ocudu {
namespace fapi {

/// Adds logging information over the implemented interface.
class logging_p7_slot_indication_notifier_decorator : public p7_slot_indication_notifier
{
public:
  explicit logging_p7_slot_indication_notifier_decorator(unsigned sector_id_, ocudulog::basic_logger& logger_);

  // See interface for documentation.
  void on_slot_indication(const slot_indication& msg) override;

  /// Sets the slot time message notifier to the given one.
  void set_p7_slot_indication_notifier(p7_slot_indication_notifier& p7_slot_ind_notifier);

private:
  const unsigned               sector_id;
  ocudulog::basic_logger&      logger;
  p7_slot_indication_notifier* p7_notifier;
};

} // namespace fapi
} // namespace ocudu
