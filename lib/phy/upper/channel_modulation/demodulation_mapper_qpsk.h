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
#include "ocudu/phy/upper/log_likelihood_ratio.h"

namespace ocudu {

/// \brief Soft-demodulates QPSK modulation.
/// \param[out] llrs       Resultant log-likelihood ratios.
/// \param[in]  symbols    Input constellation symbols.
/// \param[in]  noise_vars Noise variance for each symbol in the constellation.
void demodulate_soft_QPSK(span<log_likelihood_ratio> llrs, span<const cf_t> symbols, span<const float> noise_vars);

} // namespace ocudu
