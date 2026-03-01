// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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
