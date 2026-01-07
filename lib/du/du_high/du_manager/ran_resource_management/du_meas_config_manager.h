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

#include "du_ue_resource_config.h"
#include "ocudu/adt/byte_buffer.h"
#include "ocudu/ocudulog/logger.h"

namespace ocudu::odu {

/// Class that handles the measConfig of the UE.
///
/// In particular, this class determines the measGaps that the need to be set for the UE based on the measConfig
/// provided by the CU-CP for the UE.
class du_meas_config_manager
{
public:
  du_meas_config_manager(span<const du_cell_config> cell_cfg_list);

  /// Update UE config based on UE measConfig given by the CU-CP.
  void update(du_ue_resource_config& ue_cfg, const byte_buffer& meas_cfg);

private:
  span<const du_cell_config> cell_cfg_list;
  ocudulog::basic_logger&    logger;
};

} // namespace ocudu::odu
