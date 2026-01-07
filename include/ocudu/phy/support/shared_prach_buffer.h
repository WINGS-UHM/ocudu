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

#include "ocudu/support/memory_pool/bounded_object_pool.h"

namespace ocudu {

class prach_buffer;

using prach_buffer_pool   = bounded_rc_object_pool<prach_buffer>;
using shared_prach_buffer = prach_buffer_pool::ptr;

} // namespace ocudu
