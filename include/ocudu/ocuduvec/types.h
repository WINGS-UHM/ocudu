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

#include "ocudu/adt/complex.h"
#include "ocudu/adt/span.h"
#include "ocudu/support/ocudu_assert.h"
#include <cstdint>

/// \brief The ocuduvec library provides optimized SIMD commonly used operations.
///
/// The supported vector data types are:
/// - ocudu::span<cf_t>: For complex float vectors
/// - ocudu::span<float>: For float vectors
/// - ocudu::span<int16_t>: For signed 16 bit integer vectors
/// - ocudu::span<int8_t>: For signed 8 bit integer vectors
/// - ocudu::span<uint8_t>: For unsigned 8 bit integer vectors
///
/// To make the span constant use ocudu::span<const T>.

/// Asserts that both objects have the same size.
#define ocudu_ocuduvec_assert_size(X, Y)                                                                               \
  ocudu_assert(                                                                                                        \
      X.size() == Y.size(), "The size " #X " (i.e., {}) and " #Y " (i.e., {}) must be equal.", X.size(), Y.size())
