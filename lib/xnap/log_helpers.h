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

#include "ocudu/asn1/xnap/xnap.h"

namespace ocudu::ocucp {

/// \brief Log Received/Transmitted NGAP PDU.
void log_xnap_pdu(ocudulog::basic_logger& logger, bool json_log, bool is_rx, const asn1::xnap::xn_ap_pdu_c& pdu);

} // namespace ocudu::ocucp
