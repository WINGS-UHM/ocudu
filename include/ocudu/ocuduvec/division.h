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

#include "ocudu/ocuduvec/types.h"

namespace ocudu {
namespace ocuduvec {

/// \brief Element-wise division between two sequences.
///
/// The result of the division is zero if the numerator or the denominator is abnormal.
///
/// \param[out] result      Division result.
/// \param[in]  numerator   Division numerator.
/// \param[in]  denominator Division denominator.
/// \remark An assertion is triggered if \c numerator, \c denominator, and \c result do not have the same number of
/// elements.
void divide(span<float> result, span<const float> numerator, span<const float> denominator);

} // namespace ocuduvec
} // namespace ocudu
