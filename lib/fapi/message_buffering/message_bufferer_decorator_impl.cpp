/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "message_bufferer_decorator_impl.h"

using namespace ocudu;
using namespace fapi;

p7_requests_gateway& message_bufferer_decorator_impl::get_p7_requests_gateway()
{
  return dispatcher;
}

p7_slot_indication_notifier& message_bufferer_decorator_impl::get_p7_slot_indication_notifier_from_this_decorator()
{
  return time_notifier;
}

p7_indications_notifier& message_bufferer_decorator_impl::get_p7_indications_notifier_from_this_decorator()
{
  return p7_notifier;
}

p7_last_request_notifier& message_bufferer_decorator_impl::get_p7_last_request_notifier()
{
  return p7_last_msg_notifier;
}

error_indication_notifier& message_bufferer_decorator_impl::get_error_indication_notifier_from_this_decorator()
{
  return error_notifier;
}

void message_bufferer_decorator_impl::set_p7_slot_indication_notifier(p7_slot_indication_notifier& notifier)
{
  time_notifier.set_p7_slot_indication_notifier(notifier);
}

void message_bufferer_decorator_impl::set_p7_indications_notifier(p7_indications_notifier& notifier)
{
  p7_notifier.set_p7_indications_notifier(notifier);
}

void message_bufferer_decorator_impl::set_error_indication_notifier(error_indication_notifier& notifier)
{
  error_notifier.set_error_indication_notifier(notifier);
}
