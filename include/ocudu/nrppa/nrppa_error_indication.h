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

#include "ocudu/ran/cause/nrppa_cause.h"
#include "ocudu/ran/crit_diagnostics.h"
#include <optional>

namespace ocudu {
namespace ocucp {

struct nrppa_error_indication {
  nrppa_cause_t                     cause;
  std::optional<crit_diagnostics_t> crit_diagnostics;
};

} // namespace ocucp
} // namespace ocudu
