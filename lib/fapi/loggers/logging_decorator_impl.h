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

#include "decorator_helpers/p7_last_request_notifier_dispatcher.h"
#include "logging_error_notifier_decorator.h"
#include "logging_p7_indications_notifier_decorator.h"
#include "logging_p7_requests_gateway_decorator.h"
#include "logging_p7_slot_indication_notifier_decorator.h"
#include "ocudu/fapi/decorator.h"

namespace ocudu {
namespace fapi {

class logging_decorator_impl : public fapi_decorator
{
public:
  logging_decorator_impl(unsigned                  sector_id,
                         ocudulog::basic_logger&   logger,
                         p7_requests_gateway&      p7_gateway_,
                         p7_last_request_notifier& p7_last_req_notifier_);

  logging_decorator_impl(unsigned                        sector_id,
                         ocudulog::basic_logger&         logger,
                         std::unique_ptr<fapi_decorator> next_decorator_);

  // See interface for documentation.
  p7_last_request_notifier& get_p7_last_request_notifier() override;

  // See interface for documentation.
  p7_requests_gateway& get_p7_requests_gateway() override;

  // See interface for documentation.
  void set_p7_indications_notifier(p7_indications_notifier& notifier) override;

  // See interface for documentation.
  void set_error_indication_notifier(error_indication_notifier& notifier) override;

  // See interface for documentation.
  void set_p7_slot_indication_notifier(p7_slot_indication_notifier& notifier) override;

private:
  // See interface for documentation.
  p7_indications_notifier& get_p7_indications_notifier_from_this_decorator() override;

  // See interface for documentation.
  error_indication_notifier& get_error_indication_notifier_from_this_decorator() override;

  // See interface for documentation.
  p7_slot_indication_notifier& get_p7_slot_indication_notifier_from_this_decorator() override;

private:
  logging_p7_indications_notifier_decorator     data_notifier;
  logging_error_notifier_decorator              error_notifier;
  logging_p7_slot_indication_notifier_decorator time_notifier;
  p7_last_request_notifier_dispatcher           last_msg_notifier;
  logging_p7_requests_gateway_decorator         gateway;
};

} // namespace fapi
} // namespace ocudu
