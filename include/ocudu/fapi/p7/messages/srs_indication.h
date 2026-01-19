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

#include "ocudu/adt/static_vector.h"
#include "ocudu/fapi/common/base_message.h"
#include "ocudu/ran/phy_time_unit.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/ran/srs/srs_channel_matrix.h"
#include "ocudu/ran/srs/srs_configuration.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// Encodes SRS positioning report.
struct srs_positioning_report {
  /// TUL-RTOA as defined in TS 38.215 on section 5.1.
  std::optional<phy_time_unit> ul_relative_toa;
  std::optional<float>         rsrp;
};

/// SRS indication pdu.
struct srs_indication_pdu {
  uint32_t                              handle = 0;
  rnti_t                                rnti;
  std::optional<phy_time_unit>          timing_advance_offset;
  std::optional<srs_channel_matrix>     matrix;
  std::optional<srs_positioning_report> positioning;
};

/// SRS indication message.
struct srs_indication : public base_message {
  slot_point                                                  slot;
  static_vector<srs_indication_pdu, MAX_SRS_PDUS_PER_SRS_IND> pdus;
};

} // namespace fapi
} // namespace ocudu
