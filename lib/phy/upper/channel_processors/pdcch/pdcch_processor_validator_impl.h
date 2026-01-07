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

#include "ocudu/phy/upper/channel_processors/pdcch/pdcch_processor.h"
#include "ocudu/ran/pdcch/pdcch_constants.h"

namespace ocudu {

/// Implements a parameter validator for \ref pdcch_processor_impl.
class pdcch_processor_validator_impl : public pdcch_pdu_validator
{
public:
  // See interface for documentation.
  error_type<std::string> is_valid(const pdcch_processor::pdu_t& pdu) const override;
};

} // namespace ocudu
