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

#include "ocudu/ran/plmn_identity.h"
#include "ocudu/ran/tac.h"

namespace ocudu {

/// 3GPP TS 38.413 section 9.3.3.11.
struct tai_t {
  plmn_identity plmn_id = plmn_identity::test_value();
  tac_t         tac;
};

} // namespace ocudu
