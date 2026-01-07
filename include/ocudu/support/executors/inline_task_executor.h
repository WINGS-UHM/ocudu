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

#include "ocudu/support/executors/task_executor.h"

namespace ocudu {

class inline_task_executor final : public task_executor
{
public:
  [[nodiscard]] bool execute(unique_task task) override
  {
    task();
    return true;
  }

  [[nodiscard]] bool defer(unique_task task) override
  {
    task();
    return true;
  }
};

} // namespace ocudu
