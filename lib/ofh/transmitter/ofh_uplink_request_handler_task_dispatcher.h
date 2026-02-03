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

#include "ofh_uplink_request_handler_impl.h"
#include "ocudu/ofh/ofh_controller.h"
#include "ocudu/support/executors/task_executor.h"
#include "ocudu/support/synchronization/stop_event.h"

namespace ocudu {
namespace ofh {

/// Uplink request handler task dispatcher.
class uplink_request_handler_task_dispatcher : public uplink_request_handler, operation_controller
{
  const unsigned          sector_id;
  ocudulog::basic_logger& logger;
  uplink_request_handler& uplink_handler;
  task_executor&          executor;
  rt_stop_event_source    stop_manager;

public:
  uplink_request_handler_task_dispatcher(unsigned                sector_id_,
                                         ocudulog::basic_logger& logger_,
                                         uplink_request_handler& uplink_handler_,
                                         task_executor&          executor_) :
    sector_id(sector_id_), logger(logger_), uplink_handler(uplink_handler_), executor(executor_)
  {
  }

  // See interface for documentation.
  void start() override;

  // See interface for documentation.
  void stop() override;

  // See interface for documentation.
  void handle_prach_occasion(const prach_buffer_context& context, shared_prach_buffer buffer) override;

  // See interface for documentation.
  void handle_new_uplink_slot(const resource_grid_context& context, const shared_resource_grid& grid) override;
};

} // namespace ofh
} // namespace ocudu
