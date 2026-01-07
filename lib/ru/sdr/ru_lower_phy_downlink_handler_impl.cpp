/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_lower_phy_downlink_handler_impl.h"
#include "ocudu/phy/lower/lower_phy_downlink_handler.h"
#include "ocudu/phy/support/shared_resource_grid.h"

using namespace ocudu;

void ru_lower_phy_downlink_handler_impl::handle_dl_data(const resource_grid_context& context,
                                                        const shared_resource_grid&  grid)
{
  ocudu_assert(context.sector < handlers.size(), "Invalid sector {}", context.sector);

  handlers[context.sector]->handle_resource_grid(context, grid);
}
