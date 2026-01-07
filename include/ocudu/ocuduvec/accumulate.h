/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief Cumulative sum of sequences.

#pragma once

#include "ocudu/adt/complex.h"
#include "ocudu/adt/span.h"
#include <numeric>

namespace ocudu {
namespace ocuduvec {

///@{
/// \brief Cumulative sum of a sequence.
/// \param[in] x  Input sequence.
/// \return The sum of all elements of the input sequence.
float accumulate(span<const float> x);

inline cf_t accumulate(span<const cf_t> x)
{
  return std::accumulate(x.begin(), x.end(), cf_t());
}
///@}

} // namespace ocuduvec
} // namespace ocudu
