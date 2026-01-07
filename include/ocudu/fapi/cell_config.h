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

#include "ocudu/fapi/p5/config_request_tlvs.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/prach/rach_config_common.h"
#include "ocudu/ran/ssb/ssb_configuration.h"
#include "ocudu/ran/tdd/tdd_ul_dl_config.h"
#include <any>

namespace ocudu {
namespace fapi {

/// FAPI cell configuration.
struct cell_configuration {
  subcarrier_spacing                     scs_common;
  cyclic_prefix                          cp;
  pci_t                                  pci;
  duplex_mode                            duplex;
  carrier_config                         carrier_cfg;
  rach_config_common                     prach_cfg;
  ssb_configuration                      ssb_cfg;
  std::optional<tdd_ul_dl_config_common> tdd_ul_dl_cfg_common;
  /// Vendor specific configuration.
  std::any vendor_cfg;
};

} // namespace fapi
} // namespace ocudu
