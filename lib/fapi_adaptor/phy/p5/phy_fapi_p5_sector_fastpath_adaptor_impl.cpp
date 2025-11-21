/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "phy_fapi_p5_sector_fastpath_adaptor_impl.h"

using namespace ocudu;
using namespace fapi_adaptor;

phy_fapi_p5_sector_fastpath_adaptor_impl::phy_fapi_p5_sector_fastpath_adaptor_impl(
    const phy_fapi_p5_sector_fastpath_adaptor_config&       config,
    const phy_fapi_p5_sector_fastpath_adaptor_dependencies& dependencies) :
  gateway(config, dependencies)
{
}

fapi::config_message_gateway& phy_fapi_p5_sector_fastpath_adaptor_impl::get_config_message_gateway()
{
  return gateway;
}

void phy_fapi_p5_sector_fastpath_adaptor_impl::set_config_message_notifier(
    fapi::config_message_notifier& config_notifier)
{
  gateway.set_config_message_notifier(config_notifier);
}

void phy_fapi_p5_sector_fastpath_adaptor_impl::set_error_message_notifier(fapi::error_message_notifier& err_notifier)
{
  gateway.set_error_message_notifier(err_notifier);
}
