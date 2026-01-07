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

#include "ocudu/adt/byte_buffer_chain.h"

namespace ocudu {
namespace odu {

/// \brief This interface represents the data entry point of the transmitting side of a F1-U bearer of the DU.
/// The lower layer will use this class to pass NR-U SDUs (e.g. PDCP PDUs/RLC SDUs) into the F1-U bearer towards CU-UP.
class f1u_tx_sdu_handler
{
public:
  virtual ~f1u_tx_sdu_handler() = default;

  virtual void handle_sdu(byte_buffer_chain sdu) = 0;
  virtual void flush_ul_buffer()                 = 0;
};

} // namespace odu
} // namespace ocudu
