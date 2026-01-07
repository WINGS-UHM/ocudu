/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "logging_decorator_impl.h"

using namespace ocudu;
using namespace fapi;

logging_decorator_impl::logging_decorator_impl(unsigned                  sector_id,
                                               ocudulog::basic_logger&   logger,
                                               p7_requests_gateway&      p7_gateway_,
                                               p7_last_request_notifier& p7_last_req_notifier_) :
  fapi_decorator({}),
  data_notifier(sector_id, logger),
  error_notifier(sector_id, logger),
  time_notifier(sector_id, logger),
  gateway(sector_id, logger, p7_gateway_)
{
  last_msg_notifier.set_p7_last_request_notifier(p7_last_req_notifier_);
}

logging_decorator_impl::logging_decorator_impl(unsigned                        sector_id,
                                               ocudulog::basic_logger&         logger,
                                               std::unique_ptr<fapi_decorator> next_decorator_) :
  fapi_decorator({std::move(next_decorator_)}),
  data_notifier(sector_id, logger),
  error_notifier(sector_id, logger),
  time_notifier(sector_id, logger),
  gateway(sector_id, logger, next_decorator->get_p7_requests_gateway())
{
  last_msg_notifier.set_p7_last_request_notifier(next_decorator->get_p7_last_request_notifier());
  connect_notifiers();
}

p7_indications_notifier& logging_decorator_impl::get_p7_indications_notifier_from_this_decorator()
{
  return data_notifier;
}

p7_last_request_notifier& logging_decorator_impl::get_p7_last_request_notifier()
{
  return last_msg_notifier;
}

p7_requests_gateway& logging_decorator_impl::get_p7_requests_gateway()
{
  return gateway;
}

error_indication_notifier& logging_decorator_impl::get_error_indication_notifier_from_this_decorator()
{
  return error_notifier;
}

p7_slot_indication_notifier& logging_decorator_impl::get_p7_slot_indication_notifier_from_this_decorator()
{
  return time_notifier;
}

void logging_decorator_impl::set_p7_indications_notifier(p7_indications_notifier& notifier)
{
  data_notifier.set_p7_indications_notifier(notifier);
}

void logging_decorator_impl::set_error_indication_notifier(error_indication_notifier& notifier)
{
  error_notifier.set_error_indication_notifier(notifier);
}

void logging_decorator_impl::set_p7_slot_indication_notifier(p7_slot_indication_notifier& notifier)
{
  time_notifier.set_p7_slot_indication_notifier(notifier);
}
