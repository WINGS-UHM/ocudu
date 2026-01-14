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

namespace srs_constants {

/// Maximum number of transmit ports that can transmit Sounding Reference Signals (SRS).
constexpr unsigned max_nof_tx_ports = 4;

/// Maximum number of receive ports that can be used to receive Sounding Reference Signals (SRS).
constexpr unsigned max_nof_rx_ports = 4;

/// Maximum value of the Slot Offset \c slotOffset, as per \c SRS-ResourceSet, \c SRS-Config, TS 38.331.
constexpr unsigned MAX_SRS_SLOT_OFFSET = 32;

} // namespace srs_constants

} // namespace ocudu
