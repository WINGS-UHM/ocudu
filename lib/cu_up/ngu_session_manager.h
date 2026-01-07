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

#include "ocudu/gtpu/gtpu_gateway.h"

namespace ocudu::ocuup {

class ngu_session_manager
{
public:
  virtual ~ngu_session_manager()                       = default;
  virtual gtpu_tnl_pdu_session& get_next_ngu_gateway() = 0;
};

} // namespace ocudu::ocuup
