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

#include "ocudu/support/ocudu_assert.h"
#include "ocudu/support/tracing/event_tracing.h"
#include <string_view>

namespace ocudu {
namespace app_services {

/// \brief Application tracer service.
///
/// Manages opening/closing a trace file.
class application_tracer
{
  bool                    enabled = false;
  ocudulog::basic_logger* logger  = nullptr;

public:
  /// Enables the tracer using the given filename.
  void enable_tracer(std::string_view        file_name,
                     unsigned                split_after_n,
                     unsigned                event_trigger_n,
                     ocudulog::basic_logger& logger_)
  {
    ocudu_assert(!enabled, "Tracer service already enabled");
    logger  = &logger_;
    enabled = true;
    logger_.info("Opening event tracer in file '{}' ...", file_name);
    open_trace_file(file_name, split_after_n, event_trigger_n);
    logger_.info("Event tracer opened successfully");
  }

  ~application_tracer()
  {
    if (enabled) {
      ocudu_assert(logger, "Invalid logger");
      logger->info("Closing event tracer...");
      close_trace_file();
      logger->info("Event tracer closed successfully");
    }
  }
};

} // namespace app_services
} // namespace ocudu
