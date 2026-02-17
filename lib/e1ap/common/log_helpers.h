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

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/e1ap/common/e1ap_message.h"
#include "ocudu/e1ap/common/e1ap_types.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/cu_up_types.h"

namespace ocudu {

/// \brief Helper for logging Rx/Tx E1AP PDUs for the CU-CP and CU-UP
template <typename UeIndex>
void log_e1ap_pdu(ocudulog::basic_logger&       logger,
                  bool                          is_rx,
                  const std::optional<UeIndex>& ue_id,
                  const e1ap_message&           e1ap_msg,
                  bool                          json_enabled);

extern template void log_e1ap_pdu<ocucp::ue_index_t>(ocudulog::basic_logger&                 logger,
                                                     bool                                    is_rx,
                                                     const std::optional<ocucp::ue_index_t>& ue_id,
                                                     const e1ap_message&                     e1ap_msg,
                                                     bool                                    json_enabled);
extern template void log_e1ap_pdu<cu_up_ue_index_t>(ocudulog::basic_logger&                logger,
                                                    bool                                   is_rx,
                                                    const std::optional<cu_up_ue_index_t>& ue_id,
                                                    const e1ap_message&                    e1ap_msg,
                                                    bool                                   json_enabled);

} // namespace ocudu
