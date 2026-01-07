/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/instrumentation/traces/du_traces.h"

ocudu::file_event_tracer<ocudu::L1_DL_TRACE_ENABLED> ocudu::l1_common_tracer;

ocudu::file_event_tracer<ocudu::L1_DL_TRACE_ENABLED> ocudu::l1_dl_tracer;

ocudu::file_event_tracer<ocudu::L1_UL_TRACE_ENABLED> ocudu::l1_ul_tracer;

ocudu::file_event_tracer<ocudu::L2_TRACE_ENABLED> ocudu::l2_tracer;
