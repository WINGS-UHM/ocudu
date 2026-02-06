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

#include "ocudu/cu_cp/cu_cp_types.h"
#include "fmt/base.h"

namespace ocudu::ocucp {

struct ngap_location_reporting_control {
  enum class event_type {
    direct,
    change_of_serve_cell,
    ue_presence_in_area_of_interest,
    stop_change_of_serve_cell,
    stop_ue_presence_in_area_of_interest,
    cancel_location_report_for_the_ue,
    // ...
    change_of_serving_cell_and_ue_presence_in_the_area_of_interest,
    nulltype
  };
  event_type location_reporting_type;
};

struct ngap_location_report {
  ue_index_t          ue_index = ue_index_t::invalid;
  nr_cell_global_id_t nr_cgi;
  tai_t               tai;
};

struct ngap_location_report_failure_indication {
  ngap_cause_t cause;
};

} // namespace ocudu::ocucp

template <>
struct fmt::formatter<ocudu::ocucp::ngap_location_reporting_control::event_type> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::ocucp::ngap_location_reporting_control::event_type& cfg, FormatContext& ctx) const
  {
    std::string_view str;
    switch (cfg) {
      case ocudu::ocucp::ngap_location_reporting_control::event_type::direct:
        str = "direct";
        break;
      case ocudu::ocucp::ngap_location_reporting_control::event_type::change_of_serve_cell:
        str = "change_of_serve_cell";
        break;
      case ocudu::ocucp::ngap_location_reporting_control::event_type::ue_presence_in_area_of_interest:
        str = "ue_presence_in_area_of_interest";
        break;
      case ocudu::ocucp::ngap_location_reporting_control::event_type::stop_change_of_serve_cell:
        str = "stop_change_of_serve_cell";
        break;
      case ocudu::ocucp::ngap_location_reporting_control::event_type::stop_ue_presence_in_area_of_interest:
        str = "stop_ue_presence_in_area_of_interest";
        break;
      case ocudu::ocucp::ngap_location_reporting_control::event_type::cancel_location_report_for_the_ue:
        str = "cancel_location_report_for_the_ue";
        break;
      case ocudu::ocucp::ngap_location_reporting_control::event_type::
          change_of_serving_cell_and_ue_presence_in_the_area_of_interest:
        str = "change_of_serving_cell_and_ue_presence_in_the_area_of_interest";
        break;
      default:
        str = "nulltype";
    }
    return fmt::format_to(ctx.out(), "{}", str);
  }
};
