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

namespace ocudu {
namespace ocucp {

struct ngap_dl_nas_transport_message {
  ue_index_t  ue_index = ue_index_t::invalid;
  byte_buffer nas_pdu;
  bool        ue_cap_info_request = false;
};

} // namespace ocucp
} // namespace ocudu
