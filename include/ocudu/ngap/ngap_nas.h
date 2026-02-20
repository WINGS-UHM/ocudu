// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

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
