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

#include "ocudu/ran/cause/e1ap_cause.h"
#include "ocudu/ran/cause/ngap_cause.h"

namespace ocudu {

/// \brief Converts an E1AP cause to an NGAP cause.
ngap_cause_t e1ap_to_ngap_cause(e1ap_cause_t e1ap_cause);

} // namespace ocudu
