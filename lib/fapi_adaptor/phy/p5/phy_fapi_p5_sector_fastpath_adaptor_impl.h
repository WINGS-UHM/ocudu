/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "p5_requests_handler.h"
#include "ocudu/fapi_adaptor/phy/p5/phy_fapi_p5_sector_adaptor.h"

namespace ocudu {
namespace fapi_adaptor {

/// PHY-FAPI P5 sector fastpath adaptor implementation.
class phy_fapi_p5_sector_fastpath_adaptor_impl : public phy_fapi_p5_sector_adaptor
{
public:
  phy_fapi_p5_sector_fastpath_adaptor_impl(const phy_fapi_p5_sector_fastpath_adaptor_config&       config,
                                           const phy_fapi_p5_sector_fastpath_adaptor_dependencies& dependencies);

  // See interface for documentation.
  fapi::config_message_gateway& get_config_message_gateway() override;

  // See interface for documentation.
  void set_config_message_notifier(fapi::config_message_notifier& config_notifier) override;

  // See interface for documentation.
  void set_error_message_notifier(fapi::error_message_notifier& err_notifier) override;

private:
  p5_requests_handler gateway;
};

} // namespace fapi_adaptor
} // namespace ocudu
