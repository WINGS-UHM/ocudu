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
#include "ocudu/ran/cause/f1ap_cause.h"
#include "ocudu/ran/cause/ngap_cause.h"

namespace ocudu {

/// \brief Converts an NGAP cause to an F1AP cause.
f1ap_cause_t ngap_to_f1ap_cause(ngap_cause_t ngap_cause);

/// \brief Converts an NGAP cause to an E1AP cause.
e1ap_cause_t ngap_to_e1ap_cause(ngap_cause_t ngap_cause);

} // namespace ocudu
