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

#include "ocudu/asn1/nrppa/nrppa.h"

namespace ocudu::ocucp {

// Logging
typedef enum { Rx = 0, Tx } direction_t;

void log_nrppa_message(ocudulog::basic_logger&          logger,
                       const direction_t                dir,
                       byte_buffer_view                 pdu,
                       const asn1::nrppa::nr_ppa_pdu_c& nrppa_pdu);

} // namespace ocudu::ocucp
