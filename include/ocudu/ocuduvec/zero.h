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

#include "ocudu/ocuduvec/type_traits.h"
#include "ocudu/ocuduvec/types.h"

namespace ocudu {
namespace ocuduvec {

/// \brief Sets all elements of a sequence to zero.
///
/// \tparam T Type of the sequence container, must be span-compatible.
/// \param  x Sequence container.
template <typename T>
void zero(T&& x)
{
  static_assert(is_span_compatible<T>::value, "Template type is not compatible with a span.");
  std::fill(x.begin(), x.end(), 0);
}

} // namespace ocuduvec
} // namespace ocudu
