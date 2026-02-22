/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "du_ue_cond_mobility_manager.h"

using namespace ocudu;
using namespace ocudu::odu;

void du_ue_cond_mobility_manager::set_success_access_required()
{
  is_success_access_required = true;
}

bool du_ue_cond_mobility_manager::handle_crnti_ce_indication()
{
  if (!is_success_access_required) {
    return false;
  }
  is_success_access_required = false;
  return true;
}
