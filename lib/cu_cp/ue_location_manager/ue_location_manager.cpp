/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ue_location_manager.h"

using namespace ocudu;
using namespace ocudu::ocucp;

ue_location_manager::ue_location_manager() : logger(ocudulog::fetch_basic_logger("CU-CP")) {}

void ue_location_manager::configure_location_reporting(const ngap_location_report_request& ctrl)
{
  switch (ctrl.location_reporting_type) {
    case ngap_location_report_request::event_type::direct:
      // Direct reports are handled by the caller without configuration change.
      return;
    case ngap_location_report_request::event_type::change_of_serve_cell:
      report_on_cell_change = true;
      break;
    case ngap_location_report_request::event_type::stop_change_of_serve_cell:
      report_on_cell_change = false;
      return;
    case ngap_location_report_request::event_type::ue_presence_in_area_of_interest:
      report_ue_presence_in_aoi = true;
      break;
    case ngap_location_report_request::event_type::stop_ue_presence_in_area_of_interest:
      report_ue_presence_in_aoi = false;
      if (ctrl.location_report_ref_id_to_be_cancelled.has_value()) {
        area_of_interest_list.erase(ctrl.location_report_ref_id_to_be_cancelled.value());
      }
      for (const auto& ref_id : ctrl.additional_location_report_ref_ids_to_be_cancelled) {
        area_of_interest_list.erase(ref_id);
      }
      return;
    case ngap_location_report_request::event_type::change_of_serving_cell_and_ue_presence_in_the_area_of_interest:
      report_on_cell_change     = true;
      report_ue_presence_in_aoi = true;
      break;
    case ngap_location_report_request::event_type::cancel_location_report_for_the_ue:
      report_on_cell_change     = false;
      report_ue_presence_in_aoi = false;
      area_of_interest_list.clear();
      return;
    default:
      logger.warning("Unhandled location reporting event type: {}", ctrl.location_reporting_type);
      return;
  }

  // Add/update area of interest entries if list was sent by AMF.
  for (const auto& item : ctrl.area_of_interest_list) {
    auto [it, inserted] = area_of_interest_list.emplace(item.location_report_ref_id, item.area_of_interest);
    if (!inserted) {
      // TODO: send LocationReportingFailureIndication instead of overriding.
      logger.warning("Location report ref_id={} already exists - overriding", item.location_report_ref_id);
      it->second = item.area_of_interest;
    }
  }
}

ngap_location_report_request::event_type ue_location_manager::get_current_location_reporting_type() const
{
  if (report_on_cell_change && report_ue_presence_in_aoi) {
    return ngap_location_report_request::event_type::change_of_serving_cell_and_ue_presence_in_the_area_of_interest;
  }
  if (report_on_cell_change) {
    return ngap_location_report_request::event_type::change_of_serve_cell;
  }
  if (report_ue_presence_in_aoi) {
    return ngap_location_report_request::event_type::ue_presence_in_area_of_interest;
  }
  return ngap_location_report_request::event_type::nulltype;
}

ngap_location_report ue_location_manager::get_location_report(ue_index_t                         ue_index,
                                                              const cu_cp_user_location_info_nr& user_location_info)
{
  ngap_location_report report;
  report.ue_index           = ue_index;
  report.user_location_info = user_location_info;

  report.request.location_reporting_type = get_current_location_reporting_type();
  report.request.location_report_area    = ngap_location_report_request::report_area::cell;
  for (const auto& [ref_id, aoi] : area_of_interest_list) {
    ngap_area_of_interest_item item;
    item.location_report_ref_id = ref_id;
    item.area_of_interest       = aoi;
    report.request.area_of_interest_list.push_back(item);
  }

  return report;
}
