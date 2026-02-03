/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "slot_point_extender_adaptor.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
#include "ocudu/ran/slot_point_extended.h"

using namespace ocudu;

static unsigned get_hfn_from_time(std::chrono::microseconds slot_duration, uint64_t nof_slots_in_hyper_frame)
{
  // Get the time since the epoch.
  auto time_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch());

  return ((time_since_epoch / slot_duration) / nof_slots_in_hyper_frame) % NOF_HYPER_SFNS;
}

void slot_point_extender_adaptor::on_slot_indication(const fapi::slot_indication& msg)
{
  // Calculate the hyper frame index.
  unsigned hfn = get_hfn_from_time(slot_duration, msg.slot.nof_slots_per_hyper_system_frame());

  // Generate and notify a new SLOT.indication with the hyper frame index.
  notifier.on_slot_indication(fapi::slot_indication{.slot = slot_point_extended(msg.slot.without_hyper_sfn(), hfn),
                                                    .time_point = msg.time_point});
}
