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

#include "ocudu/adt/bounded_integer.h"

namespace ocudu::ocucp {

/// Conditional Reconfiguration ID used by CHO procedures. Range is 1..8.
using cond_recfg_id_t = bounded_integer<uint8_t, 1, 8>;

} // namespace ocudu::ocucp
