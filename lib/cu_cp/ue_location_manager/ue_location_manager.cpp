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
      cfg.report_on_cell_change = true;
      break;
    case ngap_location_report_request::event_type::stop_change_of_serve_cell:
      cfg.report_on_cell_change = false;
      return;
    case ngap_location_report_request::event_type::ue_presence_in_area_of_interest:
      cfg.report_ue_presence_in_aoi = true;
      break;
    case ngap_location_report_request::event_type::stop_ue_presence_in_area_of_interest:
      cfg.report_ue_presence_in_aoi = false;
      if (ctrl.location_report_ref_id_to_be_cancelled.has_value()) {
        cfg.area_of_interest_list.erase(ctrl.location_report_ref_id_to_be_cancelled.value());
      }
      for (const auto& ref_id : ctrl.additional_location_report_ref_ids_to_be_cancelled) {
        cfg.area_of_interest_list.erase(ref_id);
      }
      return;
    case ngap_location_report_request::event_type::change_of_serving_cell_and_ue_presence_in_the_area_of_interest:
      cfg.report_on_cell_change     = true;
      cfg.report_ue_presence_in_aoi = true;
      break;
    case ngap_location_report_request::event_type::cancel_location_report_for_the_ue:
      cfg.report_on_cell_change     = false;
      cfg.report_ue_presence_in_aoi = false;
      cfg.area_of_interest_list.clear();
      return;
    default:
      logger.warning("Unhandled location reporting event type: {}", ctrl.location_reporting_type);
      return;
  }

  // Add/update area of interest entries if list was sent by AMF.
  for (const auto& item : ctrl.area_of_interest_list) {
    auto [it, inserted] = cfg.area_of_interest_list.emplace(item.location_report_ref_id, item.area_of_interest);
    if (!inserted) {
      // TODO: send LocationReportingFailureIndication instead of overriding.
      logger.warning("Location report ref_id={} already exists - overriding", item.location_report_ref_id);
      it->second = item.area_of_interest;
    }
  }
}

ngap_location_report_request::event_type ue_location_manager::get_current_location_reporting_type() const
{
  if (cfg.report_on_cell_change && cfg.report_ue_presence_in_aoi) {
    return ngap_location_report_request::event_type::change_of_serving_cell_and_ue_presence_in_the_area_of_interest;
  }
  if (cfg.report_on_cell_change) {
    return ngap_location_report_request::event_type::change_of_serve_cell;
  }
  if (cfg.report_ue_presence_in_aoi) {
    return ngap_location_report_request::event_type::ue_presence_in_area_of_interest;
  }
  return ngap_location_report_request::event_type::nulltype;
}

ngap_ue_presence ue_location_manager::check_ue_presence(const ngap_area_of_interest&       aoi,
                                                        const cu_cp_user_location_info_nr& loc)
{
  for (const auto& cell : aoi.cell_list) {
    // TODO: add handling for other types of CGIs
    if (cell == loc.nr_cgi) {
      return ngap_ue_presence::in;
    }
  }

  for (const auto& tai : aoi.tai_list) {
    if (tai.plmn_id == loc.tai.plmn_id && tai.tac == loc.tai.tac) {
      return ngap_ue_presence::in;
    }
  }

  if (!aoi.ran_node_list.empty()) {
    // TODO: add handling for other types of RAN Nodes, take gNB ID from NR-CGI?
    return ngap_ue_presence::unknown;
  }

  return ngap_ue_presence::out;
}

ngap_location_report ue_location_manager::get_location_report(ue_index_t                         ue_index,
                                                              const cu_cp_user_location_info_nr& user_location_info)
{
  ngap_location_report report;
  report.ue_index           = ue_index;
  report.user_location_info = user_location_info;

  report.request.location_reporting_type = get_current_location_reporting_type();
  report.request.location_report_area    = ngap_location_report_request::report_area::cell;
  for (const auto& [ref_id, aoi] : cfg.area_of_interest_list) {
    ngap_area_of_interest_item item;
    item.location_report_ref_id = ref_id;
    item.area_of_interest       = aoi;
    report.request.area_of_interest_list.push_back(item);
  }

  if (cfg.report_ue_presence_in_aoi && !cfg.area_of_interest_list.empty()) {
    report.ue_presence_in_area_of_interest_list.emplace();
    for (const auto& [ref_id, aoi] : cfg.area_of_interest_list) {
      ngap_ue_presence_in_area_of_interest_item item;
      item.location_report_ref_id = ref_id;
      item.ue_presence            = check_ue_presence(aoi, user_location_info);
      report.ue_presence_in_area_of_interest_list->push_back(item);
    }
  }

  return report;
}
