// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/common/error_indication_notifier.h"
#include "ocudu/ocudulog/ocudulog.h"

namespace ocudu {
namespace fapi {

/// Adds logging information over the implemented interface.
class logging_error_notifier_decorator : public error_indication_notifier
{
public:
  logging_error_notifier_decorator(unsigned sector_id_, ocudulog::basic_logger& logger_);

  // See interface for documentation.
  void on_error_indication(const error_indication& msg) override;

  /// Sets the error indication notifier to the given one.
  void set_error_indication_notifier(error_indication_notifier& error_notifier);

private:
  /// Sector identifier.
  const unsigned sector_id;
  /// FAPI logger.
  ocudulog::basic_logger& logger;
  /// Error notifier.
  error_indication_notifier* notifier;
};

} // namespace fapi
} // namespace ocudu
