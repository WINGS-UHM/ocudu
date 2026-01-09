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

#include "ocudu/support/async/async_task.h"
#include "ocudu/support/async/eager_async_task.h"
#include "ocudu/support/synchronization/sync_event.h"

namespace ocudu {

/// Wait for the completion of the provided task in a blocking manner.
template <typename Result>
Result sync_await(async_task<Result> t)
{
  Result     resp;
  sync_event ev;

  // This task binds the lifetime of the sync_token to the completion of the provided task.
  auto launcher = launch_async([&resp, &t, tk = ev.get_token()](coro_context<eager_async_task<void>>& ctx) {
    CORO_BEGIN(ctx);
    CORO_AWAIT_VALUE(resp, t);
    CORO_RETURN();
  });

  // Block waiting for procedure completion.
  ev.wait();
  ocudu_assert(launcher.ready(), "sync_await return without task completion");

  return resp;
}

} // namespace ocudu
