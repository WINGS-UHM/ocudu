// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/cu_cp/cu_cp_xnc_handler.h"
#include "ocudu/xnap/xnap.h"

namespace ocudu::ocucp {

/// Adapter between NGAP and CU-CP
class xnap_cu_cp_adapter : public xnap_cu_cp_notifier
{
public:
  void connect_cu_cp(cu_cp_xnc_handler& cu_cp_handler_) { cu_cp_handler = &cu_cp_handler_; }

private:
  cu_cp_xnc_handler* cu_cp_handler = nullptr;
};

} // namespace ocudu::ocucp
