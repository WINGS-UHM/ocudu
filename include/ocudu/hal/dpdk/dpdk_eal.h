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
#include <rte_eal.h>

namespace ocudu {
namespace dpdk {

/// Interfacing to DPDK's EAL.
class dpdk_eal
{
public:
  /// Constructor.
  /// \param[in] logger OCUDU logger.
  explicit dpdk_eal(ocudulog::basic_logger& logger_) : logger(logger_) {}

  /// Destructor.
  ~dpdk_eal()
  {
    // Clean up the EAL.
    ::rte_eal_cleanup();
  }

  // Returns the internal OCUDU logger.
  /// \return OCUDU logger.
  ocudulog::basic_logger& get_logger() { return logger; }

private:
  /// OCUDU logger.
  ocudulog::basic_logger& logger;
};

} // namespace dpdk
} // namespace ocudu
