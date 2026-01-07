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

#include "ocudu/asn1/e1ap/e1ap.h"

namespace ocudu {

/// \brief E1AP message transferred between a CU-CP and a CU-UP.
struct e1ap_message {
  asn1::e1ap::e1ap_pdu_c pdu;
};

} // namespace ocudu
