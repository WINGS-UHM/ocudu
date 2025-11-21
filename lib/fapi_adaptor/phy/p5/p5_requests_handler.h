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

#include "ocudu/fapi/p5/config_message_gateway.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/support/synchronization/stop_event.h"

namespace ocudu {
class task_executor;

class upper_phy_operation_controller;

namespace fapi {
class config_message_notifier;
class error_message_notifier;
} // namespace fapi

namespace fapi_adaptor {

struct phy_fapi_p5_sector_fastpath_adaptor_config;
struct phy_fapi_p5_sector_fastpath_adaptor_dependencies;

/// \brief FAPI P5 requests handler.
///
/// This class process incoming FAPI P5 request messages (PARAM.request, CONFIG.request, START.request and STOP.request)
/// by dispatching the start/stop to the upper PHY operation controller and generating dummy responses for the
/// param/config requests.
class p5_requests_handler : public fapi::config_message_gateway
{
public:
  p5_requests_handler(const phy_fapi_p5_sector_fastpath_adaptor_config&       config,
                      const phy_fapi_p5_sector_fastpath_adaptor_dependencies& dependencies);

  ~p5_requests_handler() override;

  // See interface for documentation.
  void param_request(const fapi::param_request& msg) override;

  // See interface for documentation.
  void config_request(const fapi::config_request& msg) override;

  // See interface for documentation.
  void start_request(const fapi::start_request& msg) override;

  // See interface for documentation.
  void stop_request(const fapi::stop_request& msg) override;

  /// Sets the config message notifier of this gateway.
  void set_config_message_notifier(fapi::config_message_notifier& cfg_notifier) { config_notifier = &cfg_notifier; }

  /// Sets the error message notifier of this gateway.
  void set_error_message_notifier(fapi::error_message_notifier& err_notifier) { error_notifier = &err_notifier; }

private:
  const unsigned                  sector;
  ocudulog::basic_logger&         logger;
  task_executor&                  executor;
  upper_phy_operation_controller& upper_phy_controller;
  fapi::config_message_notifier*  config_notifier = nullptr;
  fapi::error_message_notifier*   error_notifier  = nullptr;
  stop_event_source               stop_manager;
};

} // namespace fapi_adaptor
} // namespace ocudu
