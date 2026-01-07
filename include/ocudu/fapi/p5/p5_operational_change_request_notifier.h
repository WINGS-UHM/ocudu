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

namespace ocudu {
namespace fapi {

struct cell_configuration;

/// \brief P5 FAPI operational change request notifier.
///
/// Notifies that P5 FAPI START/STOP request messages have been processed and requests a change in the operational state
/// of the cell.
class p5_operational_change_request_notifier
{
public:
  virtual ~p5_operational_change_request_notifier() = default;

  /// \brief Notifies a start request from FAPI.
  ///
  /// This method is synchronous, as such, when this callback returns the start procedure will be completed. Returns
  /// true on start success, otherwise false.
  virtual bool on_start_request(const cell_configuration& config) = 0;

  /// \brief Notifies a stop request from FAPI.
  ///
  /// This method is synchronous, so when this function returns the stop process is already finished.
  virtual void on_stop_request() = 0;
};

} // namespace fapi
} // namespace ocudu
