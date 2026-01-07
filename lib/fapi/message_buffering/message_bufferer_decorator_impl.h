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

#include "decorator_helpers/error_notifier_dispatcher.h"
#include "decorator_helpers/p7_indications_notifier_dispatcher.h"
#include "decorator_helpers/p7_last_request_notifier_dispatcher.h"
#include "message_bufferer_slot_gateway_task_dispatcher.h"
#include "message_bufferer_slot_time_notifier_decorator.h"
#include "ocudu/fapi/decorator.h"

namespace ocudu {
namespace fapi {

/// \brief FAPI message bufferer decorator.
///
/// The message bufferer caches FAPI messages that are send by the L2 and forwards them to the L1 when the
/// SLOT.indication that matches the FAPI message arrives.
class message_bufferer_decorator_impl : public fapi_decorator
{
public:
  message_bufferer_decorator_impl(unsigned                  sector_id,
                                  unsigned                  l2_nof_slots_ahead,
                                  subcarrier_spacing        scs,
                                  p7_requests_gateway&      p7_gateway,
                                  p7_last_request_notifier& p7_last_req_notifier_,
                                  task_executor&            executor) :
    fapi_decorator({}),
    dispatcher(sector_id, l2_nof_slots_ahead, scs, p7_gateway, executor),
    time_notifier(l2_nof_slots_ahead, scs, dispatcher)
  {
    p7_last_msg_notifier.set_p7_last_request_notifier(p7_last_req_notifier_);
  }

  // See interface for documentation.
  p7_requests_gateway& get_p7_requests_gateway() override;

  // See interface for documentation.
  p7_last_request_notifier& get_p7_last_request_notifier() override;

  // See interface for documentation.
  void set_p7_slot_indication_notifier(p7_slot_indication_notifier& notifier) override;

  // See interface for documentation.
  void set_p7_indications_notifier(p7_indications_notifier& notifier) override;

  // See interface for documentation.
  void set_error_indication_notifier(error_indication_notifier& notifier) override;

private:
  // See interface for documentation.
  p7_slot_indication_notifier& get_p7_slot_indication_notifier_from_this_decorator() override;

  // See interface for documentation.
  p7_indications_notifier& get_p7_indications_notifier_from_this_decorator() override;

  // See interface for documentation.
  error_indication_notifier& get_error_indication_notifier_from_this_decorator() override;

private:
  message_bufferer_slot_gateway_task_dispatcher dispatcher;
  message_bufferer_slot_time_notifier_decorator time_notifier;
  p7_last_request_notifier_dispatcher           p7_last_msg_notifier;
  p7_indications_notifier_dispatcher            p7_notifier;
  error_notifier_dispatcher                     error_notifier;
};

} // namespace fapi
} // namespace ocudu
