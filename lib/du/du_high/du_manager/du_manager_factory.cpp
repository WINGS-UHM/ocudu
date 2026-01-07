/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/du/du_high/du_manager/du_manager_factory.h"
#include "du_manager_impl.h"

using namespace ocudu;
using namespace odu;

std::unique_ptr<du_manager_interface> ocudu::odu::create_du_manager(const du_manager_params& params)
{
  auto du_manager = std::make_unique<du_manager_impl>(params);
  return du_manager;
}
