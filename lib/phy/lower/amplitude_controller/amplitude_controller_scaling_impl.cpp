/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "amplitude_controller_scaling_impl.h"
#include "ocudu/ocuduvec/sc_prod.h"

using namespace ocudu;

amplitude_controller_metrics amplitude_controller_scaling_impl::process(span<cf_t> output, span<const cf_t> input)
{
  ocudu_ocuduvec_assert_size(output, input);

  // Apply scaling factor.
  ocuduvec::sc_prod(output, input, amplitude_gain);

  // Return empty metrics.
  return {0.0F, 0.0F, 0.0F, 0.0F, 0UL, 0UL, 0.0, false};
}
