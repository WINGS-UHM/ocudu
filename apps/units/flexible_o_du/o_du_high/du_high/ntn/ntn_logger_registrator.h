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

#include "ocudu/ocudulog/logger.h"
#include "ocudu/ocudulog/ocudulog.h"

namespace ocudu {

/// Registers the NTN loggers in the logger service.
inline void register_ntn_loggers(const ocudulog::basic_levels& ntn_level)
{
  auto& ntn_logger = ocudulog::fetch_basic_logger("NTN", true);
  ntn_logger.set_level(ntn_level);
}

} // namespace ocudu
