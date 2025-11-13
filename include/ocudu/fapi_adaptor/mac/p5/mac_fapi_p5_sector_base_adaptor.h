/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

namespace ocudu {

namespace fapi {
class config_message_gateway;
class config_message_notifier;
class error_message_notifier;
} // namespace fapi

namespace fapi_adaptor {

/// \brief MAC-FAPI P5 sector base adaptor interface.
///
/// This adaptor manages the FAPI P5 messages and procedures that allows to configure and start/stop an L1 instance.
///
/// Note: This interface is considered a base as it lacks the operation controller that starts/stops the adaptor.
/// Note: This interface is not intended to be used on its self, it only serves as a common point for the other MAC-FAPI
/// P5 sector adaptor interfaces that contain an operation controller.
class mac_fapi_p5_sector_base_adaptor
{
public:
  virtual ~mac_fapi_p5_sector_base_adaptor() = default;

  /// Returns the FAPI configuration message notifier of this adaptor.
  virtual fapi::config_message_notifier& get_config_message_notifier() = 0;

  /// Returns the FAPI error message notifier of this adaptor.
  virtual fapi::error_message_notifier& get_error_message_notifier() = 0;
};

} // namespace fapi_adaptor
} // namespace ocudu
