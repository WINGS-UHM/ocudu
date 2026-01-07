/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ru_ofh_controller_impl.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;

void ru_ofh_controller_impl::start()
{
  logger.info("Starting the operation of the Open Fronthaul interface");
  for (auto* sector : sector_controllers) {
    sector->start();
  }
  logger.info("Started the operation of the Open Fronthaul interface");
}

void ru_ofh_controller_impl::stop()
{
  logger.info("Stopping the operation of the Open Fronthaul interface");
  for (auto* sector : sector_controllers) {
    sector->stop();
  }
  logger.info("Stopped the operation of the Open Fronthaul interface");
}

void ru_ofh_controller_impl::set_sector_controllers(std::vector<ofh::operation_controller*> controllers)
{
  ocudu_assert(!controllers.empty(), "Invalid sector controllers");

  sector_controllers = std::move(controllers);

  ocudu_assert(std::all_of(sector_controllers.begin(),
                           sector_controllers.end(),
                           [](const auto& elem) { return elem != nullptr; }),
               "Invalid sector controller");
}
