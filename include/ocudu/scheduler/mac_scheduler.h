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

#include "ocudu/scheduler/scheduler_configurator.h"
#include "ocudu/scheduler/scheduler_dl_buffer_state_indication_handler.h"
#include "ocudu/scheduler/scheduler_feedback_handler.h"
#include "ocudu/scheduler/scheduler_paging_handler.h"
#include "ocudu/scheduler/scheduler_positioning_handler.h"
#include "ocudu/scheduler/scheduler_slot_handler.h"
#include "ocudu/scheduler/scheduler_sys_info_handler.h"

namespace ocudu {

class mac_scheduler : public scheduler_configurator,
                      public scheduler_ue_configurator,
                      public scheduler_feedback_handler,
                      public scheduler_slot_handler,
                      public scheduler_dl_buffer_state_indication_handler,
                      public scheduler_paging_handler,
                      public scheduler_sys_info_handler,
                      public scheduler_positioning_handler
{
public:
  virtual ~mac_scheduler() = default;
};

} // namespace ocudu
