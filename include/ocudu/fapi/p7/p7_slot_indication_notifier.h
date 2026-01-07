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

struct slot_indication;

/// \brief P7 slot indication message notifier interface.
///
/// This interface notifies the reception of slot indications from the underlying PHY.
class p7_slot_indication_notifier
{
public:
  virtual ~p7_slot_indication_notifier() = default;

  /// \brief Notifies the reception of a slot indication message.
  ///
  /// \param[in] msg Message contents.
  virtual void on_slot_indication(const slot_indication& msg) = 0;
};

} // namespace fapi
} // namespace ocudu
