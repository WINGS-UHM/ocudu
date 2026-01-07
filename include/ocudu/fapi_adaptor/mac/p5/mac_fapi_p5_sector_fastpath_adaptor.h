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

#include "ocudu/fapi_adaptor/mac/p5/mac_fapi_p5_sector_base_adaptor.h"

namespace ocudu {

class phy_cell_operation_controller;

namespace fapi_adaptor {

/// \brief MAC-FAPI P5 sector fastpath adaptor interface.
///
/// The fastpath adaptor is used by the MAC layer to start/stop the cell.
class mac_fapi_p5_sector_fastpath_adaptor : public mac_fapi_p5_sector_base_adaptor
{
public:
  /// Returns the PHY cell operation controller of this adaptor.
  virtual phy_cell_operation_controller& get_operation_controller() = 0;
};

} // namespace fapi_adaptor
} // namespace ocudu
