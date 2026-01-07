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

#include "ocudu/adt/bounded_bitset.h"
#include "ocudu/ran/uci/uci_constants.h"

namespace ocudu {

/// Generic UCI payload type.
using uci_payload_type = bounded_bitset<uci_constants::MAX_NOF_PAYLOAD_BITS>;

} // namespace ocudu
