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

struct error_indication;

/// \brief Error-specific message notifier interface.
///
/// This interface listens to FAPI error indication events and translates them to the suitable data types
/// for the MAC layer.
class error_indication_notifier
{
public:
  virtual ~error_indication_notifier() = default;

  /// \brief Notifies the reception of an error indication message.
  ///
  /// \param[in] msg Message contents.
  virtual void on_error_indication(const error_indication& msg) = 0;
};

} // namespace fapi
} // namespace ocudu
