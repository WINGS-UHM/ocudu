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
#include "ocudu/gateways/baseband/baseband_gateway_timestamp.h"

namespace ocudu {

class baseband_gateway_buffer_reader;

/// \brief Lower physical layer uplink processor - Baseband interface.
///
/// Processes baseband samples. It derives the symbol and slot timing from the number of processed samples.
class uplink_processor_baseband
{
public:
  /// Default destructor.
  virtual ~uplink_processor_baseband() = default;

  /// \brief Processes any number of baseband samples.
  ///
  /// \param[in] buffer    Baseband samples to process.
  /// \param[in] timestamp Time instant in which the first sample was captured.
  /// \remark The number of channels in \c buffer must be equal to the number of receive ports for the sector.
  virtual void process(const baseband_gateway_buffer_reader& buffer, baseband_gateway_timestamp timestamp) = 0;
};

} // namespace ocudu
