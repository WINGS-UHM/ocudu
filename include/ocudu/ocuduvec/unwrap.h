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
/// \brief Phase angle unwrapping declarations.

#pragma once

#include "ocudu/adt/complex.h"
#include "ocudu/adt/span.h"

namespace ocudu {
namespace ocuduvec {

/// \brief Computes the unwrapped arguments (phase angles) of a list of complex numbers.
///
/// \param[out] arguments     Output list of unwrapped arguments.
/// \param[in]  cplx_numbers  Input list of complex numbers.
///
/// \warning The input and output lists should have the same length.
/// \warning The memory allocated for input and output should be disjoint.
void unwrap_arguments(span<float> arguments, span<const cf_t> cplx_numbers);

} // namespace ocuduvec
} // namespace ocudu
