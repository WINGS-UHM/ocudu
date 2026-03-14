// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "du_manager_controller_impl.h"
#include "du_manager_context.h"
#include "procedures/du_setup_procedure.h"
#include "procedures/du_stop_procedure.h"
#include "ocudu/support/async/fifo_async_task_scheduler.h"
#include "ocudu/support/executors/task_executor.h"
#include "ocudu/support/ocudu_assert.h"

using namespace ocudu;
using namespace odu;

du_manager_controller_impl::du_manager_controller_impl(const du_proc_context_view& proc_ctxt_,
                                                       fifo_async_task_scheduler&  task_sched_) :
  main_task_sched(task_sched_), proc_ctxt(proc_ctxt_)
{
}

void du_manager_controller_impl::start()
{
  // Check if not already running.
  if (running_guard_flag) {
    proc_ctxt.logger.warning("Discarding DU start request. Cause: DU Manager already started.");
    return;
  }
  running_guard_flag = true;

  sync_event ev;
  if (not proc_ctxt.params.services.du_mng_exec.execute([this, tk = ev.get_token()]() mutable {
        main_task_sched.schedule([this, tk = std::move(tk)](coro_context<async_task<void>>& ctx) {
          CORO_BEGIN(ctx);

          // Connect to CU-CP and send F1 Setup Request and await for F1 setup response.
          CORO_AWAIT(launch_async<du_setup_procedure>(proc_ctxt));

          // Update DU state to "running".
          proc_ctxt.ctxt.running = true;

          // On tk destruction, caller thread that the operation is complete.
          CORO_RETURN();
        });
      })) {
    report_fatal_error("Unable to initiate DU setup procedure");
  }

  // Block waiting for DU setup to complete.
  ev.wait();
  ocudu_sanity_check(running_guard_flag, "DU manager start()/stop() being used in an non-sequential manner");
}

void du_manager_controller_impl::stop()
{
  if (not running_guard_flag) {
    // Stop was already requested. Do nothing.
    return;
  }
  running_guard_flag = false;

  sync_event ev;
  while (not proc_ctxt.params.services.du_mng_exec.execute(
      [this, tk = ev.get_token()]() mutable { handle_du_stop_request(std::move(tk)); })) {
    proc_ctxt.logger.error("Unable to dispatch DU Manager shutdown. Retrying...");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Block waiting for async task to complete.
  ev.wait();
  ocudu_sanity_check(not running_guard_flag, "DU manager start()/stop() being used in an non-sequential manner");
}

void du_manager_controller_impl::handle_du_stop_request(scoped_sync_token tk)
{
  if (not proc_ctxt.ctxt.running) {
    // Already stopped.
    return;
  }

  // Notify other procedures that the DU needs to stop.
  // Note: If the DU was in the process of being setup, this may cancel the procedure.
  proc_ctxt.ctxt.stop_command_received = true;

  // Start DU stop procedure.
  main_task_sched.schedule(launch_async([this, tk = std::move(tk)](coro_context<async_task<void>>& ctx) mutable {
    CORO_BEGIN(ctx);

    if (not proc_ctxt.ctxt.running) {
      // Already stopped.
      CORO_EARLY_RETURN();
    }

    // Tear down activity in remaining layers.
    CORO_AWAIT(launch_async<du_stop_procedure>(proc_ctxt.ue_mng, proc_ctxt.cell_mng, proc_ctxt.params.f1ap.conn_mng));

    // DU stop successfully finished.
    // Dispatch main async task loop destruction via defer so that the current coroutine ends successfully before
    // we start destroying the DU manager (in case stop is called from dtor).
    while (not proc_ctxt.params.services.du_mng_exec.defer([this, tk = std::move(tk)]() {
      // Let main loop go out of scope and be destroyed.
      auto main_loop = main_task_sched.request_stop();

      proc_ctxt.ctxt.running               = false;
      proc_ctxt.ctxt.stop_command_received = false;
    })) {
      proc_ctxt.logger.warning("Unable to stop DU Manager. Retrying...");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    CORO_RETURN();
  }));
}
