// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

namespace ocudu::ocucp {

struct rrc_ue_capabilities_t {
  bool rrc_inactive_supported = false;

  // TS 38.306 Section 4.2.7.2 - CHO capabilities are reported per-band but the spec mandates that a UE sets each value
  // consistently across all bands within the same band group (FDD-FR1, TDD-FR1, TDD-FR2-1, TDD-FR2-2). A single boolean
  // is therefore sufficient: the flag is set to true as soon as any band advertises support.

  /// UE supports CHO including execution condition, candidate cell configuration, and up to 8 candidate cells.
  bool conditional_handover_supported = false;

  /// UE supports 2 trigger events for the same execution condition. Mandatory if condHandover-r16 is supported.
  bool conditional_handover_two_trigger_events_supported = false;

  /// UE supports Event A4 based CHO (CondEvent-A4 per TS 38.331). A UE supporting this shall also indicate
  /// condHandover-r16 for NTN bands and nonTerrestrialNetwork-r17 support.
  bool conditional_handover_event_a4_supported = false;

  /// UE supports location-based CHO (CondEvent-D1 per TS 38.331). A UE supporting this shall also indicate
  /// condHandover-r16 for NTN bands and nonTerrestrialNetwork-r17 support.
  bool conditional_handover_location_based_supported = false;

  /// UE supports time-based CHO (CondEvent-T1 per TS 38.331). A UE supporting this shall also indicate condHandover-r16
  /// for NTN bands and nonTerrestrialNetwork-r17 support.
  bool conditional_handover_time_based_supported = false;
};

} // namespace ocudu::ocucp
