/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_positioning_handler_factory.h"
#include "du_positioning_manager_impl.h"

using namespace ocudu;
using namespace odu;

std::unique_ptr<f1ap_du_positioning_handler> odu::create_du_positioning_handler(const du_manager_params& du_params,
                                                                                du_cell_manager&         cell_mng,
                                                                                du_ue_manager&           ue_mng,
                                                                                ocudulog::basic_logger&  logger)
{
  return std::make_unique<du_positioning_manager_impl>(du_params, cell_mng, ue_mng, logger);
}
