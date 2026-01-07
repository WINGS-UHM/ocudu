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

#include "ocudu/ran/paging_information.h"

namespace ocudu {

/// \brief Interface to handle paging information from the CU-CP.
class mac_paging_information_handler
{
public:
  virtual ~mac_paging_information_handler() = default;

  /// \brief Handles Paging information.
  ///
  /// \param msg Information of the paging message to schedule.
  virtual void handle_paging_information(const paging_information& msg) = 0;
};

} // namespace ocudu
