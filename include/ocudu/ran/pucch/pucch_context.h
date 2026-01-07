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

#include "ocudu/ran/rnti.h"

namespace ocudu {

/// PUCCH transmission context.
class pucch_context
{
public:
  /// Default constructor.
  pucch_context() = default;

  /// Constructs from value.
  explicit pucch_context(rnti_t rnti_) : rnti(rnti_) {}

private:
  friend struct fmt::formatter<pucch_context>;
  rnti_t rnti = rnti_t::INVALID_RNTI;
};

} // namespace ocudu
