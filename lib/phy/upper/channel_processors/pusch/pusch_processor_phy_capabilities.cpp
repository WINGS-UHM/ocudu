/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/phy/upper/channel_processors/pusch/pusch_processor_phy_capabilities.h"

using namespace ocudu;

#ifndef PUSCH_PROCESSOR_MAX_NOF_LAYERS
#define PUSCH_PROCESSOR_MAX_NOF_LAYERS 4
#endif // PUSCH_PROCESSOR_MAX_NOF_LAYERS

pusch_processor_phy_capabilities ocudu::get_pusch_processor_phy_capabilities()
{
  return {.max_nof_layers = PUSCH_PROCESSOR_MAX_NOF_LAYERS};
}
