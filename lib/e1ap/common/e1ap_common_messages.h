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

#include "ocudu/asn1/e1ap/e1ap_ies.h"
#include "ocudu/e1ap/common/e1ap_message.h"
#include "ocudu/e1ap/common/e1ap_types.h"

namespace ocudu {

/// Generate an E1AP Error Indication message.
e1ap_message generate_error_indication(uint8_t                                      transaction_id,
                                       const std::optional<gnb_cu_up_ue_e1ap_id_t>& cu_up_ue_id,
                                       const std::optional<gnb_cu_cp_ue_e1ap_id_t>& cu_cp_ue_id,
                                       const std::optional<asn1::e1ap::cause_c>&    cause);

} // namespace ocudu
