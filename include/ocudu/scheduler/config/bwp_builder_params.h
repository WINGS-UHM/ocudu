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

namespace ocudu {

/// Parameters used to generate a BWP configuration.
struct bwp_builder_params {
  /// Minimum k1 supported by the BWP for both common and dedicated PUCCH. Possible values: {1, ..., 7}.
  /// [Implementation-defined] Even though the "dl-DataToUl-Ack" ranges from {1, ..., 15} as per TS 38.213, 9.1.2.1, we
  /// restrict min_k1 to be at most 7. The reason being that we need a k1 < 8 for common PUCCH allocations.
  uint8_t min_k1 = 2;
  /// \brief Minimum k2 value used in the generation of the UE PUSCH time-domain resources.
  /// Possible values: {1, ..., 7}.
  /// [Implementation-defined] The value of min_k2 should be equal or lower than min_k1. Otherwise, the scheduler is
  /// forced to pick higher k1 values, as it cannot allocate PUCCHs in slots where there is an PUSCH with an already
  /// assigned DAI.
  uint8_t min_k2 = 2;
};

} // namespace ocudu
