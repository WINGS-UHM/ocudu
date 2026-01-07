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

#include "ocudu/fapi_adaptor/phy/phy_fapi_fastpath_adaptor.h"
#include "ocudu/fapi_adaptor/phy/phy_fapi_sector_fastpath_adaptor.h"
#include <memory>
#include <vector>

namespace ocudu {
namespace fapi_adaptor {

/// PHY-FAPI bidirectional fastpath adaptor implementation.
class phy_fapi_fastpath_adaptor_impl : public phy_fapi_fastpath_adaptor
{
public:
  explicit phy_fapi_fastpath_adaptor_impl(
      std::vector<std::unique_ptr<phy_fapi_sector_fastpath_adaptor>> sector_adaptors_);

  // See interface for documentation.
  phy_fapi_sector_fastpath_adaptor& get_sector_adaptor(unsigned cell_id) override;

private:
  std::vector<std::unique_ptr<phy_fapi_sector_fastpath_adaptor>> sector_adaptors;
};

} // namespace fapi_adaptor
} // namespace ocudu
