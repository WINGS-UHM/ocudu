/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "p5_requests_handler.h"
#include "ocudu/fapi/common/error_message_notifier.h"
#include "ocudu/fapi/p5/config_message_notifier.h"
#include "ocudu/fapi/p5/config_messages.h"
#include "ocudu/fapi_adaptor/phy/p5/phy_fapi_p5_sector_fastpath_adaptor_config.h"
#include "ocudu/phy/upper/upper_phy_operation_controller.h"
#include "ocudu/support/executors/task_executor.h"

using namespace ocudu;
using namespace fapi_adaptor;

namespace {

class config_message_notifier_dummy : public fapi::config_message_notifier
{
public:
  void on_config_response(const fapi::config_response& msg) override {}
  void on_param_response(const fapi::param_response& msg) override {}
  void on_stop_indication(const fapi::stop_indication& msg) override {}
};

class error_message_notifier_dummy : public fapi::error_message_notifier
{
public:
  void on_error_indication(const fapi::error_indication_message& msg) override {}
};

} // namespace

static config_message_notifier_dummy dummy_config_notifier;
static error_message_notifier_dummy  dummy_error_notifier;

p5_requests_handler::p5_requests_handler(const phy_fapi_p5_sector_fastpath_adaptor_config&       config,
                                         const phy_fapi_p5_sector_fastpath_adaptor_dependencies& dependencies) :
  sector(config.sector_id),
  logger(dependencies.logger),
  executor(dependencies.executor),
  upper_phy_controller(dependencies.upper_phy_controller),
  config_notifier(&dummy_config_notifier),
  error_notifier(&dummy_error_notifier)
{
}

p5_requests_handler::~p5_requests_handler()
{
  stop_manager.stop();
}

void p5_requests_handler::param_request(const fapi::param_request& msg)
{
  fapi::param_response response;
  response.error_code = fapi::error_code_id::msg_ok;

  config_notifier->on_param_response(response);
}

void p5_requests_handler::config_request(const fapi::config_request& msg)
{
  fapi::config_response response;
  response.error_code = fapi::error_code_id::msg_ok;

  config_notifier->on_config_response(response);
}

void p5_requests_handler::start_request(const fapi::start_request& msg)
{
  if (!executor.defer([this, token = stop_manager.get_token()]() { upper_phy_controller.start(); })) {
    logger.warning("Sector #{}: PHY-FAPI P5 sector adaptor failed to enqueue start task ", sector);
  }
}

void p5_requests_handler::stop_request(const fapi::stop_request& msg)
{
  if (!executor.defer([this, token = stop_manager.get_token()]() {
        upper_phy_controller.stop();

        fapi::stop_indication indication;
        config_notifier->on_stop_indication(indication);
      })) {
    logger.warning("Sector #{}: PHY-FAPI P5 sector adaptor failed to enqueue stop task ", sector);
  }
}
