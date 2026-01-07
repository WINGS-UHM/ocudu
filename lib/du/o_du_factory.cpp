/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/du/o_du_factory.h"
#include "o_du_impl.h"
#include "ocudu/ocudulog/ocudulog.h"

using namespace ocudu;
using namespace odu;

std::unique_ptr<o_du> odu::make_o_du(o_du_dependencies&& dependencies)
{
  ocudulog::fetch_basic_logger("DU").info("O-DU created successfully");

  return std::make_unique<o_du_impl>(std::move(dependencies));
}
