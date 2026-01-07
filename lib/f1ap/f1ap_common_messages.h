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

#include "ocudu/asn1/f1ap/f1ap_ies.h"
#include "ocudu/f1ap/f1ap_message.h"
#include "ocudu/f1ap/f1ap_ue_id_types.h"

namespace ocudu {

/// Generate an F1AP Error Indication message.
f1ap_message generate_error_indication(uint8_t                                   transaction_id,
                                       const std::optional<gnb_du_ue_f1ap_id_t>& du_ue_id,
                                       const std::optional<gnb_cu_ue_f1ap_id_t>& cu_ue_id,
                                       const std::optional<asn1::f1ap::cause_c>& cause);

} // namespace ocudu
