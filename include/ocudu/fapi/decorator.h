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

#include <memory>

namespace ocudu {
namespace fapi {

class error_indication_notifier;
class p7_indications_notifier;
class p7_last_request_notifier;
class p7_requests_gateway;
class p7_slot_indication_notifier;

/// FAPI decorator interface.
class fapi_decorator
{
public:
  explicit fapi_decorator(std::unique_ptr<fapi_decorator> next_decorator_) : next_decorator(std::move(next_decorator_))
  {
  }

  virtual ~fapi_decorator() = default;

  /// Returns the P7 last request notifier of this FAPI decorator.
  virtual p7_last_request_notifier& get_p7_last_request_notifier() = 0;

  /// Returns the P7 requests gateway of this FAPI decorator.
  virtual p7_requests_gateway& get_p7_requests_gateway() = 0;

  /// Returns the P7 indications notifier of this FAPI decorator.
  p7_indications_notifier& get_p7_indications_notifier()
  {
    return next_decorator ? next_decorator->get_p7_indications_notifier()
                          : get_p7_indications_notifier_from_this_decorator();
  }

  /// Returns the slot error indication notifier of this FAPI decorator.
  error_indication_notifier& get_error_indication_notifier()
  {
    return next_decorator ? next_decorator->get_error_indication_notifier()
                          : get_error_indication_notifier_from_this_decorator();
  }

  /// Returns the P7 slot indication notifier of this FAPI decorator.
  p7_slot_indication_notifier& get_p7_slot_indication_notifier()
  {
    return next_decorator ? next_decorator->get_p7_slot_indication_notifier()
                          : get_p7_slot_indication_notifier_from_this_decorator();
  }

  /// Sets the P7 indications notifier of this FAPI decorator.
  virtual void set_p7_indications_notifier(p7_indications_notifier& notifier) = 0;

  /// Sets the error indication notifier of this FAPI decorator.
  virtual void set_error_indication_notifier(error_indication_notifier& notifier) = 0;

  /// Sets the P7 slot indication notifier of this FAPI decorator.
  virtual void set_p7_slot_indication_notifier(p7_slot_indication_notifier& notifier) = 0;

protected:
  /// Connects the next decorator notifiers to this FAPI decorator.
  void connect_notifiers()
  {
    next_decorator->set_p7_indications_notifier(get_p7_indications_notifier_from_this_decorator());
    next_decorator->set_error_indication_notifier(get_error_indication_notifier_from_this_decorator());
    next_decorator->set_p7_slot_indication_notifier(get_p7_slot_indication_notifier_from_this_decorator());
  }

private:
  /// Returns the P7 indications notifier of this FAPI decorator.
  virtual p7_indications_notifier& get_p7_indications_notifier_from_this_decorator() = 0;

  /// Returns the error indication notifier of this FAPI decorator.
  virtual error_indication_notifier& get_error_indication_notifier_from_this_decorator() = 0;

  /// Returns the P7 slot indication notifier of this FAPI decorator.
  virtual p7_slot_indication_notifier& get_p7_slot_indication_notifier_from_this_decorator() = 0;

protected:
  std::unique_ptr<fapi_decorator> next_decorator = nullptr;
};

} // namespace fapi
} // namespace ocudu
