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

#include "ocudu/support/tracing/event_tracing.h"

namespace ocudu {

/// Set to true for enabling radio unit trace.
#ifndef OCUDU_RU_TRACE
constexpr bool RU_TRACE_ENABLED = false;
#else
constexpr bool RU_TRACE_ENABLED = true;
#endif

/// RU event tracing. This tracer is used to analyze latencies in the RU processing.
extern file_event_tracer<RU_TRACE_ENABLED> ru_tracer;

} // namespace ocudu
