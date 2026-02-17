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

#include "ue_context.h"
#include "ocudu/cu_up/cu_up_state.h"
#include <cstddef>

namespace ocudu {

namespace ocuup {

class ue_manager_ctrl
{
public:
  virtual ~ue_manager_ctrl() = default;

  virtual async_task<void> stop()                               = 0;
  virtual ue_context*      add_ue(const ue_context_cfg& ue_cfg) = 0;
  virtual async_task<void> remove_all_ues()                     = 0;
  virtual async_task<void> remove_ue(cu_up_ue_index_t ue_index) = 0;
  virtual ue_context*      find_ue(cu_up_ue_index_t ue_index)   = 0;
  virtual size_t           get_nof_ues() const                  = 0;
  virtual up_state_t       get_up_state() const                 = 0;
};

} // namespace ocuup

} // namespace ocudu
