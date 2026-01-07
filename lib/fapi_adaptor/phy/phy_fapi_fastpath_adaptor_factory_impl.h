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
#include "ocudu/fapi_adaptor/phy/phy_fapi_fastpath_adaptor_factory.h"
#include <memory>

namespace ocudu {

namespace fapi_adaptor {

/// Implementation of the PHY-FAPI adaptor factory.
class phy_fapi_fastpath_adaptor_factory_impl : public phy_fapi_fastpath_adaptor_factory
{
public:
  // See interface for documentation.
  std::unique_ptr<phy_fapi_fastpath_adaptor> create(const phy_fapi_fastpath_adaptor_config& config,
                                                    phy_fapi_fastpath_adaptor_dependencies  dependencies) override;
};

} // namespace fapi_adaptor
} // namespace ocudu
