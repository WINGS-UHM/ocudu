// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/support/async/async_task.h"

namespace ocudu {
namespace ocucp {

/// Scheduler of asynchronous tasks that are not associated to any specific UE.
class common_task_scheduler
{
public:
  virtual ~common_task_scheduler() = default;

  /// Schedule a new common task.
  virtual bool schedule_async_task(async_task<void> task) = 0;
};

} // namespace ocucp
} // namespace ocudu
