/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "o_cu_up_unit_impl.h"

using namespace ocudu;

o_cu_up_unit_impl::o_cu_up_unit_impl(std::vector<std::unique_ptr<gtpu_gateway>>       gateways_,
                                     std::unique_ptr<e2_cu_metrics_connector_manager> e2_metric_connector_,
                                     std::unique_ptr<ocuup::o_cu_up>                  cu_up_) :
  gateways(std::move(gateways_)), e2_metric_connector(std::move(e2_metric_connector_)), cu_up(std::move(cu_up_))
{
  ocudu_assert(not gateways.empty(), "Invalid NG-U gateway");
  ocudu_assert(cu_up, "Invalid CU-UP");
}

ocuup::cu_up_interface& o_cu_up_unit_impl::get_cu_up()
{
  return cu_up->get_cu_up();
}

ocuup::cu_up_operation_controller& o_cu_up_unit_impl::get_operation_controller()
{
  return cu_up->get_operation_controller();
}
