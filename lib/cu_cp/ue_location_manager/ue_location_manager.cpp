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

std::optional<ngap_cause_t> ue_location_manager::configure_location_reporting(const ngap_location_report_request& ctrl)
{
  using event_type = ngap_location_report_request::event_type;
  auto req_type    = ctrl.location_reporting_type;

  if (req_type == event_type::nulltype) {
    // Reject malformed requests.
    logger.error("Received malformed Location Report Request with event type = null");
    return std::nullopt;
  }

  if (req_type == event_type::direct) {
    // Ignore direct reports, those should be handled with get_direct_location_report(), not for configuration.
    logger.error("Direct type Location Report Request should not be used for location reporting configuration");
    return std::nullopt;
  }

  if (req_type == event_type::cancel_location_report_for_the_ue) {
    // Cancel location reporting for the UE.
    cfg.report_on_cell_change     = false;
    cfg.report_ue_presence_in_aoi = false;
    cfg.area_of_interest_list.clear();
  }

  if (req_type == event_type::stop_change_of_serve_cell) {
    // Cancel change of serve cell reporting for the UE.
    cfg.report_on_cell_change = false;
    return std::nullopt;
  }

  if (req_type == event_type::stop_ue_presence_in_area_of_interest) {
    // Cancel UE presence in area of interest reporting for requested report reference IDs.
    if (ctrl.location_report_ref_id_to_be_cancelled.has_value()) {
      cfg.area_of_interest_list.erase(ctrl.location_report_ref_id_to_be_cancelled.value());
    }
    for (const auto& ref_id : ctrl.additional_location_report_ref_ids_to_be_cancelled) {
      cfg.area_of_interest_list.erase(ref_id);
    }
    // Cancel UE presence in area of interest reporting for the UE if no configured areas left.
    if (cfg.area_of_interest_list.empty()) {
      cfg.report_ue_presence_in_aoi = false;
    }
    return std::nullopt;
  }

  if (req_type == event_type::change_of_serve_cell ||
      req_type == event_type::change_of_serving_cell_and_ue_presence_in_the_area_of_interest) {
    // Enable change of serve cell reporting for the UE.
    cfg.report_on_cell_change = true;
  }

  if (req_type == event_type::ue_presence_in_area_of_interest ||
      req_type == event_type::change_of_serving_cell_and_ue_presence_in_the_area_of_interest) {
    cfg.report_ue_presence_in_aoi = true;
    // Validate input, reject on ref_ids duplicated within the message or current configuration (TS 38.413 8.12.1.3).
    std::map<location_report_ref_id_t, ngap_area_of_interest> incoming;
    for (const auto& item : ctrl.area_of_interest_list) {
      if (!(incoming.emplace(item.location_report_ref_id, item.area_of_interest).second) ||
          cfg.area_of_interest_list.count(item.location_report_ref_id) != 0) {
        logger.error("Location Reporting Control contains duplicate or conflicting Location Reporting Reference IDs");
        return ngap_cause_radio_network_t::multiple_location_report_ref_id_instances;
      }
    }

    // Apply all validated entries.
    cfg.area_of_interest_list.insert(incoming.begin(), incoming.end());
  }

  return std::nullopt;
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

std::optional<ngap_location_report>
ue_location_manager::get_location_report(ue_index_t ue_index, const cu_cp_user_location_info_nr& user_location_info)
{
  if (!cfg.report_on_cell_change && !cfg.report_ue_presence_in_aoi) {
    return std::nullopt;
  }

  // Suppress report if only change_of_serve_cell reporting is active and the cell hasn't changed since the last report.
  if (cfg.report_on_cell_change && !cfg.report_ue_presence_in_aoi && cfg.last_reported_location.has_value() &&
      cfg.last_reported_location->nr_cgi == user_location_info.nr_cgi) {
    return std::nullopt;
  }

  ngap_location_report report;
  report.ue_index           = ue_index;
  report.user_location_info = user_location_info;

  // Build ngap_location_report_request that the report refers to from current configuration.
  report.request.location_reporting_type = get_current_location_reporting_type();
  report.request.location_report_area    = ngap_location_report_request::report_area::cell;
  for (const auto& [ref_id, aoi] : cfg.area_of_interest_list) {
    report.request.area_of_interest_list.push_back({aoi, ref_id});
  }

  report.ue_presence_in_area_of_interest_list = build_ue_presence_list(user_location_info);

  cfg.last_reported_location = user_location_info;
  return report;
}

ngap_location_report
ue_location_manager::get_direct_location_report(ue_index_t                          ue_index,
                                                const cu_cp_user_location_info_nr&  user_location_info,
                                                const ngap_location_report_request& request)
{
  ngap_location_report report;
  report.ue_index                             = ue_index;
  report.user_location_info                   = user_location_info;
  report.request                              = request;
  report.ue_presence_in_area_of_interest_list = build_ue_presence_list(user_location_info);
  cfg.last_reported_location                  = user_location_info;
  return report;
}

std::optional<std::vector<ngap_ue_presence_in_area_of_interest_item>>
ue_location_manager::build_ue_presence_list(const cu_cp_user_location_info_nr& user_location_info) const
{
  if (!cfg.report_ue_presence_in_aoi || cfg.area_of_interest_list.empty()) {
    return std::nullopt;
  }
  std::vector<ngap_ue_presence_in_area_of_interest_item> list;
  list.reserve(cfg.area_of_interest_list.size());
  for (const auto& [ref_id, aoi] : cfg.area_of_interest_list) {
    list.push_back({ref_id, check_ue_presence(aoi, user_location_info)});
  }
  return list;
}
