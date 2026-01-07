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

#include "ocudu/asn1/ngap/ngap.h"

namespace ocudu {
namespace ocucp {

/// \brief NGAP PDU sent and received from the AMF.
struct ngap_message {
  asn1::ngap::ngap_pdu_c pdu;
};

} // namespace ocucp
} // namespace ocudu
