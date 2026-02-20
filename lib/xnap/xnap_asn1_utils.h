// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/asn1/xnap/xnap.h"

namespace ocudu {
namespace ocucp {
namespace asn1_utils {

/// Extracts message type.
const char* get_message_type_str(const asn1::xnap::xn_ap_pdu_c& pdu);

} // namespace asn1_utils
} // namespace ocucp
} // namespace ocudu
