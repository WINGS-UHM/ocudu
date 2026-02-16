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

#include "ocudu/ran/dmrs/dmrs.h"
#include "ocudu/ran/sib/system_info_config.h"
#include "ocudu/ran/ssb/ssb_configuration.h"

namespace ocudu {

/// Parameters related with the the SSB, MIB and SIB1 and other-SI scheduling information
struct si_acquisition_info {
  /// SSB configuration.
  ssb_configuration ssb_cfg;
  /// Position of first DM-RS in Downlink, as per TS 38.211, 7.4.1.1.1.
  dmrs_typeA_position dmrs_typeA_pos;
  /// \c cellSelectionInfo, sent in \c SIB1, as per TS 38.331.
  cell_selection_info cell_sel_info;
  /// \c cellAccessRelatedInfo, sent in \c SIB1, as per TS 38.331.
  cell_access_related_info cell_acc_rel_info;
  /// Content and scheduling information of SI-messages.
  std::optional<si_scheduling_info_config> si_config;
  /// \c ueTimersAndConstants, sent in \c SIB1, as per TS 38.331.
  ue_timers_and_constants_config ue_timers_and_constants;
};

} // namespace ocudu
