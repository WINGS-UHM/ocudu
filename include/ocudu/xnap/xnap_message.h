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

/// XNAP message definition which is transferred between CU-CPs.
struct xnap_message {
  asn1::xnap::xn_ap_pdu_c pdu;
};

} // namespace ocudu::ocucp
