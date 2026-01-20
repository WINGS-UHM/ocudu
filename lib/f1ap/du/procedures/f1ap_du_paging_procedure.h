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

#include "ocudu/asn1/f1ap/f1ap.h"
#include "ocudu/ocudulog/logger.h"

namespace ocudu {
namespace odu {

class f1ap_du_paging_notifier;
struct f1ap_du_context;

/// Handles a Paging Request message as per TS38.473, Section 8.7.
bool handle_paging_request(const asn1::f1ap::paging_s& msg,
                           f1ap_du_paging_notifier&    paging_notifier,
                           f1ap_du_context&            ctxt,
                           ocudulog::basic_logger&     logger);

} // namespace odu
} // namespace ocudu
