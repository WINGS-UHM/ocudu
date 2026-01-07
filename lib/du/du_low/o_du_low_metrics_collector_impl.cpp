/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "o_du_low_metrics_collector_impl.h"
#include "ocudu/du/du_low/du_low_metrics_collector.h"
#include "ocudu/du/du_low/o_du_low_metrics.h"

using namespace ocudu;
using namespace odu;

void o_du_low_metrics_collector_impl::collect_metrics(o_du_low_metrics& metrics)
{
  if (collector == nullptr) {
    return;
  }

  // Collect DU low metrics.
  collector->collect_metrics(metrics.du_lo_metrics);
}
