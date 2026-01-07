/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/cu_cp/cu_cp_factory.h"
#include "cu_cp_impl.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;

std::unique_ptr<ocucp::cu_cp> ocudu::create_cu_cp(const ocucp::cu_cp_configuration& cfg_)
{
  return std::make_unique<ocucp::cu_cp_impl>(cfg_);
}
