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

#include "ocudu/ru/ru_downlink_plane.h"
#include "ocudu/support/ocudu_assert.h"
#include <algorithm>
#include <vector>

namespace ocudu {

class task_executor;
class shared_resource_grid;

namespace ofh {
class downlink_handler;
} // namespace ofh

/// This proxy implementation dispatches the requests to the corresponding OFH sector.
class ru_downlink_plane_handler_proxy : public ru_downlink_plane_handler
{
public:
  ru_downlink_plane_handler_proxy() = default;

  explicit ru_downlink_plane_handler_proxy(std::vector<ofh::downlink_handler*> sectors_) : sectors(std::move(sectors_))
  {
    ocudu_assert(std::all_of(sectors.begin(), sectors.end(), [](const auto& elem) { return elem != nullptr; }),
                 "Invalid sector");
  }

  // See interface for documentation.
  void handle_dl_data(const resource_grid_context& context, const shared_resource_grid& grid) override;

private:
  std::vector<ofh::downlink_handler*> sectors;
};

} // namespace ocudu
