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

#include "ocudu/f1ap/du/f1ap_du_connection_manager.h"
#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace odu {

class du_cell_manager;
class du_ue_manager;
struct du_manager_params;
class du_manager_metrics_aggregator_impl;

/// \brief Procedure to handle context update request provided by CU.
class cu_configuration_procedure
{
public:
  cu_configuration_procedure(const gnbcu_config_update_request&  request_,
                             du_cell_manager&                    cell_mng_,
                             du_ue_manager&                      ue_mng_,
                             const du_manager_params&            du_params_,
                             du_manager_metrics_aggregator_impl& metrics_);

  void operator()(coro_context<async_task<gnbcu_config_update_response>>& ctx);

private:
  async_task<bool> start_cell(const nr_cell_global_id_t& cgi);
  async_task<void> stop_cell(const nr_cell_global_id_t& cgi);

  const gnbcu_config_update_request   request;
  du_cell_manager&                    cell_mng;
  du_ue_manager&                      ue_mng;
  const du_manager_params&            du_params;
  du_manager_metrics_aggregator_impl& metrics;

  unsigned list_index = 0;

  gnbcu_config_update_response resp;
};

} // namespace odu
} // namespace ocudu
