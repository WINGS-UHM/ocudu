/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "p5_responses_handler.h"
#include "p5_transaction_outcome_manager.h"
#include "ocudu/fapi/common/error_indication.h"
#include "ocudu/fapi/p5/config_messages.h"

using namespace ocudu;
using namespace fapi_adaptor;

void p5_responses_handler::on_param_response(const fapi::param_response& msg)
{
  if (!fapi_ctrl_executor.defer([this, error_code = msg.error_code]() {
        // Set the transaction result.
        transaction_manager.param_response_outcome.set(error_code == fapi::error_code_id::msg_ok);
      })) {
    logger.warning("FAPI control executor: failed to enqueue task to set the PARAM.response awaitable");
  }
}

void p5_responses_handler::on_config_response(const fapi::config_response& msg)
{
  if (!fapi_ctrl_executor.defer([this, error_code = msg.error_code]() {
        // Set the transaction result.
        transaction_manager.config_response_outcome.set(error_code == fapi::error_code_id::msg_ok);
      })) {
    logger.warning("FAPI control executor: failed to enqueue task to set the CONFIG.response awaitable");
  }
}

void p5_responses_handler::on_stop_indication(const fapi::stop_indication& msg)
{
  if (!fapi_ctrl_executor.defer([this]() {
        // Receiving the STOP.indication message means the stop procedure finished successfully. In case of error, it
        // gets reported by an ERROR.indication.
        transaction_manager.stop_response_outcome.set(true);
      })) {
    logger.warning("FAPI control executor: failed to enqueue task to set the STOP.indication awaitable");
  }
}

void p5_responses_handler::handle_slot_indication(const mac_cell_timing_context& context)
{
  //TODO: add a proper stop procedure to flush any pending tasks in the fapi executor
  if (is_first_slot.load(std::memory_order_relaxed)) {
    is_first_slot = false;
    if (!fapi_ctrl_executor.defer([this]() { transaction_manager.start_outcome.set(true); })) {
      logger.warning("FAPI control executor: failed to enqueue task to set the START.request awaitable");
    }
  }
}

void p5_responses_handler::on_error_indication(const fapi::error_indication_message& msg)
{
  if (!fapi_ctrl_executor.defer([this, msg_id = msg.message_id]() {
        switch (msg_id) {
          case fapi::message_type_id::param_request:
            logger.warning("Dropped ERROR.indication message as PARAM.request does not trigger an ERROR.indication");
            return;
          case fapi::message_type_id::config_request:
            logger.warning("Dropped ERROR.indication message as CONFIG.request does not trigger an ERROR.indication");
            return;
          case fapi::message_type_id::start_request:
            transaction_manager.start_outcome.set(false);
            return;
          case fapi::message_type_id::stop_request:
            transaction_manager.stop_response_outcome.set(false);
            return;
          default:
            break;
        }
      })) {
    logger.warning("FAPI control executor: failed to enqueue task to set the start/stop coroutines awaitable");
  }
}
