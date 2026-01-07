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

#include "ocudu/fapi_adaptor/mac/p7/mac_fapi_p7_sector_fastpath_adaptor_config.h"

namespace ocudu {

class mac_cell_slot_handler;

namespace fapi_adaptor {

/// MAC-FAPI P7 sector fastpath adaptor implementation dependencies.
struct mac_fapi_p7_sector_fastpath_adaptor_impl_dependencies {
  mac_fapi_p7_sector_fastpath_adaptor_dependencies base_dependencies;
  mac_cell_slot_handler&                           slot_handler;
};

} // namespace fapi_adaptor
} // namespace ocudu
