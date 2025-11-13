/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "mac_fapi_p5_sector_fastpath_adaptor_impl.h"
#include "ocudu/fapi_adaptor/mac/p5/mac_fapi_p5_sector_fastpath_adaptor_config.h"

using namespace ocudu;
using namespace fapi_adaptor;

mac_fapi_p5_sector_fastpath_adaptor_impl::mac_fapi_p5_sector_fastpath_adaptor_impl(
    const mac_fapi_p5_sector_fastpath_adaptor_config&       config,
    const mac_fapi_p5_sector_fastpath_adaptor_dependencies& dependencies) :
  transaction_manager({dependencies.timers, dependencies.fapi_ctrl_executor}),
  responses_handler(dependencies.logger, transaction_manager, dependencies.fapi_ctrl_executor),
  controller(mac_fapi_p5_fastpath_cell_operation_controller_impl_config{.cell_cfg = config.cell_cfg},
             mac_fapi_p5_fastpath_cell_operation_controller_impl_dependencies{
                 .logger              = dependencies.logger,
                 .config_msg_gateway  = dependencies.gateway,
                 .transaction_manager = transaction_manager,
                 .timers              = dependencies.timers,
                 .fapi_ctrl_executor  = dependencies.fapi_ctrl_executor,
                 .mac_ctrl_executor   = dependencies.mac_ctrl_executor})
{
}

fapi::config_message_notifier& mac_fapi_p5_sector_fastpath_adaptor_impl::get_config_message_notifier()
{
  return responses_handler;
}

fapi::error_message_notifier& mac_fapi_p5_sector_fastpath_adaptor_impl::get_error_message_notifier()
{
  return responses_handler;
}

phy_cell_operation_controller& mac_fapi_p5_sector_fastpath_adaptor_impl::get_operation_controller()
{
  return controller;
}
