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

#include "ocudu/ran/du_types.h"
#include "ocudu/support/tracing/rusage_trace_recorder.h"

namespace ocudu {

/// General event tracing for critical events such as real-time errors.
extern file_event_tracer<true> general_critical_tracer;

} // namespace ocudu
