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

#include "ocudu/support/ocudu_assert.h"

namespace ocudu {

inline int tpc_mapping(unsigned tpc_command)
{
  switch (tpc_command) {
    case 0:
      return -1;
    case 1:
      return 0;
    case 2:
      return 1;
    case 3:
      return 3;
    default:
      ocudu_assertion_failure("Invalid TPC command");
      return 0;
  }
}

} // namespace ocudu
