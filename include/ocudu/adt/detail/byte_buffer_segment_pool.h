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

#include "ocudu/support/memory_pool/fixed_size_memory_block_pool.h"

namespace ocudu {
namespace detail {

/// Pool of byte buffer segments.
struct byte_buffer_pool_tag {};
using byte_buffer_segment_pool = fixed_size_memory_block_pool<byte_buffer_pool_tag>;

/// Get default byte buffer segment pool. Initialize pool if not initialized before.
detail::byte_buffer_segment_pool& get_default_byte_buffer_segment_pool();

} // namespace detail
} // namespace ocudu
