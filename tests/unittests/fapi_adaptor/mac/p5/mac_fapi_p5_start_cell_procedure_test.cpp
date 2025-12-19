/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "helpers.h"
#include "mac_fapi_p5_start_cell_procedure.h"
#include "p5_transaction_outcome_manager.h"
#include "ocudu/fapi/p5/p5_requests_gateway.h"
#include "ocudu/support/executors/task_worker.h"
#include "ocudu/support/io/io_broker_factory.h"
#include "ocudu/support/io/io_timer_source.h"
#include <gtest/gtest.h>

using namespace ocudu;
using namespace fapi_adaptor;

namespace {

class config_message_gateway_spy : public fapi::p5_requests_gateway
{
  std::atomic<bool> param_request_sent  = false;
  std::atomic<bool> config_request_sent = false;
  std::atomic<bool> start_request_sent  = false;

public:
  void send_param_request(const fapi::param_request& msg) override { param_request_sent = true; }
  void send_config_request(const fapi::config_request& msg) override { config_request_sent = true; }
  void send_stop_request(const fapi::stop_request& msg) override {}
  void send_start_request(const fapi::start_request& msg) override { start_request_sent = true; }

  bool has_param_request_been_sent() const { return param_request_sent; }
  bool has_config_request_been_sent() const { return config_request_sent; }
  bool has_start_request_been_sent() const { return start_request_sent; }
};

class mac_fapi_p5_start_cell_procedure_fixture : public ::testing::Test
{
protected:
  timer_manager timers;
  general_task_worker<concurrent_queue_policy::locking_mpsc, concurrent_queue_wait_policy::condition_variable>
      fapi_worker;
  general_task_worker_executor<concurrent_queue_policy::locking_mpsc, concurrent_queue_wait_policy::condition_variable>
      fapi_executor;
  general_task_worker<concurrent_queue_policy::locking_mpsc, concurrent_queue_wait_policy::condition_variable>
      mac_worker;
  general_task_worker_executor<concurrent_queue_policy::locking_mpsc, concurrent_queue_wait_policy::condition_variable>
                                       mac_executor;
  std::unique_ptr<io_broker>           epoll_broker;
  io_timer_source                      timer_source;
  p5_transaction_outcome_manager       transaction_manager;
  config_message_gateway_spy           gateway_spy;
  fapi::cell_configuration             cell_cfg;
  std::chrono::milliseconds            timeout{100};
  mac_fapi_start_cell_procedure_config config{cell_cfg, timeout};
  /// Procedure objects.
  async_task<bool>                                      proc;
  std::optional<unittest::waitable_task_launcher<bool>> proc_launcher;

public:
  explicit mac_fapi_p5_start_cell_procedure_fixture() :
    fapi_worker("fapi_exec_test", 32),
    fapi_executor(fapi_worker),
    mac_worker("mac_exec_test", 32),
    mac_executor(mac_worker),
    epoll_broker(create_io_broker(io_broker_type::epoll, {})),
    timer_source(timers, *epoll_broker, mac_executor, std::chrono::milliseconds{1}),
    transaction_manager(timer_factory{timers, fapi_executor})
  {
  }

  void start_procedure()
  {
    sync_event start;

    // Spawn start procedure in the MAC executor and wait until it has started.
    (void)mac_executor.defer([this, token = start.get_token()]() {
      mac_fapi_start_cell_procedure_dependencies dependencies = {.logger     = ocudulog::fetch_basic_logger("TEST"),
                                                                 .p5_gateway = gateway_spy,
                                                                 .transaction_manager = transaction_manager,
                                                                 .mac_ctrl_executor   = mac_executor,
                                                                 .fapi_ctrl_executor  = fapi_executor,
                                                                 .timers              = timers};

      proc = launch_async<mac_fapi_start_cell_procedure>(config, dependencies);
      proc_launcher.emplace(proc);
    });
  }

  void wait_procedure_to_finish()
  {
    proc_launcher->wait_for_coroutine_result();
    mac_worker.wait_pending_tasks();
  }

  void send_param_response(bool outcome = true)
  {
    while (!gateway_spy.has_param_request_been_sent()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    (void)fapi_executor.defer([this, outcome]() { transaction_manager.param_response_outcome.set(outcome); });
  }

  void send_config_response(bool outcome = true)
  {
    while (!gateway_spy.has_config_request_been_sent()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    (void)fapi_executor.defer([this, outcome]() { transaction_manager.config_response_outcome.set(outcome); });
  }

  void send_start_response(bool outcome = true)
  {
    while (!gateway_spy.has_start_request_been_sent()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    (void)fapi_executor.defer([this, outcome]() { transaction_manager.start_outcome.set(outcome); });
  }
};

} // namespace

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, param_request_is_sent_and_no_response_triggers_timeout)
{
  start_procedure();
  wait_procedure_to_finish();

  // Procedure completion checks.
  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_FALSE(gateway_spy.has_config_request_been_sent());
  ASSERT_FALSE(gateway_spy.has_start_request_been_sent());
  ASSERT_FALSE(*proc_launcher->result);
}

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, config_request_is_sent_and_no_response_triggers_timeout)
{
  start_procedure();
  send_param_response();
  wait_procedure_to_finish();

  // Procedure completion checks.
  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_TRUE(gateway_spy.has_config_request_been_sent());
  ASSERT_FALSE(gateway_spy.has_start_request_been_sent());
  ASSERT_FALSE(*proc_launcher->result);
}

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, start_request_is_sent_and_no_response_triggers_timeout)
{
  start_procedure();
  send_param_response();
  send_config_response();
  wait_procedure_to_finish();

  // Procedure completion checks.
  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_TRUE(gateway_spy.has_start_request_been_sent());
  ASSERT_FALSE(*proc_launcher->result);
}

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, start_procedure_completes_correctly)
{
  start_procedure();
  send_param_response();
  send_config_response();
  send_start_response();
  wait_procedure_to_finish();

  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_TRUE(gateway_spy.has_start_request_been_sent());
  // Check that the procedure has finished.
  ASSERT_TRUE(proc_launcher->finished());

  // Procedure completion checks.
  ASSERT_TRUE(*proc_launcher->result);
}

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, param_response_contain_error_then_procedure_returns_error)
{
  start_procedure();
  send_param_response(false);
  wait_procedure_to_finish();

  // Procedure completion checks.
  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_FALSE(gateway_spy.has_config_request_been_sent());
  ASSERT_FALSE(gateway_spy.has_start_request_been_sent());
  ASSERT_FALSE(*proc_launcher->result);
}

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, config_response_contain_error_then_procedure_returns_error)
{
  start_procedure();
  send_param_response();
  send_config_response(false);
  wait_procedure_to_finish();

  // Check that the procedure has finished.
  ASSERT_TRUE(proc_launcher->finished());

  // Procedure completion checks.
  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_TRUE(gateway_spy.has_config_request_been_sent());
  ASSERT_FALSE(gateway_spy.has_start_request_been_sent());
  ASSERT_FALSE(*proc_launcher->result);
}

TEST_F(mac_fapi_p5_start_cell_procedure_fixture, start_request_produces_error_then_procedure_returns_error)
{
  start_procedure();
  send_param_response();
  send_config_response();
  send_start_response(false);
  wait_procedure_to_finish();

  // Check that the procedure has finished.
  ASSERT_TRUE(proc_launcher->finished());

  // Procedure completion checks.
  ASSERT_TRUE(gateway_spy.has_param_request_been_sent());
  ASSERT_TRUE(gateway_spy.has_config_request_been_sent());
  ASSERT_TRUE(gateway_spy.has_start_request_been_sent());
  ASSERT_FALSE(*proc_launcher->result);
}
