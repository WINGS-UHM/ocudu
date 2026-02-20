// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

namespace ocudu::ocucp {

struct rrc_ue_capabilities_t {
  bool rrc_inactive_supported                            = false;
  bool conditional_handover_supported                    = false;
  bool conditional_handover_two_trigger_events_supported = false;
};

} // namespace ocudu::ocucp
