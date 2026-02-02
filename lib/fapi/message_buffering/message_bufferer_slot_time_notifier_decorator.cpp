/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "message_bufferer_slot_time_notifier_decorator.h"
#include "message_bufferer_slot_gateway_task_dispatcher.h"
#include "ocudu/fapi/p7/builders/slot_indication_builder.h"
#include "ocudu/fapi/p7/messages/slot_indication.h"
#include "ocudu/ran/slot_point.h"

using namespace ocudu;
using namespace fapi;

namespace {

/// Dummy slot time message notifier.
class p7_slot_indication_notifier_dummy : public p7_slot_indication_notifier
{
public:
  // See interface for documentation.
  void on_slot_indication(const slot_indication& msg) override {}
};

} // namespace

/// Dummy notifier.
static p7_slot_indication_notifier_dummy dummy_notifier;

message_bufferer_slot_time_notifier_decorator::message_bufferer_slot_time_notifier_decorator(
    unsigned                                       l2_nof_slots_ahead_,
    subcarrier_spacing                             scs_,
    message_bufferer_slot_gateway_task_dispatcher& gateway_task_dispatcher_) :
  scs(scs_),
  l2_nof_slots_ahead(l2_nof_slots_ahead_),
  l2_nof_slots_ahead_ns(l2_nof_slots_ahead * 1000000 / slot_point(scs, 0).nof_slots_per_subframe()),
  gateway_task_dispatcher(gateway_task_dispatcher_),
  notifier(dummy_notifier)
{
}

void message_bufferer_slot_time_notifier_decorator::on_slot_indication(const slot_indication& msg)
{
  // First update the current slot of the gateway task dispatcher.
  gateway_task_dispatcher.update_current_slot(msg.slot.without_hyper_sfn());

  // Notify the upper layers.
  slot_point_extended delayed_slot = msg.slot + l2_nof_slots_ahead;
  notifier.get().on_slot_indication(build_slot_indication(delayed_slot, msg.time_point + l2_nof_slots_ahead_ns));

  // Forward cached messages.
  gateway_task_dispatcher.forward_cached_messages(msg.slot.without_hyper_sfn());
}
