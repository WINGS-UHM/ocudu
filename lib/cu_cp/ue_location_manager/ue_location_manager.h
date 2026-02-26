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

#include "ocudu/ngap/ngap_location_reporting.h"
#include "ocudu/ocudulog/ocudulog.h"
#include <map>

namespace ocudu::ocucp {

using location_report_ref_id_t = uint8_t; // (1..64)

struct ue_location_manager_cfg {
  bool                                                      report_on_cell_change     = false;
  bool                                                      report_ue_presence_in_aoi = false;
  std::map<location_report_ref_id_t, ngap_area_of_interest> area_of_interest_list;
};

class ue_location_manager
{
public:
  ue_location_manager();

  /// \brief Restore location reporting state (e.g. after intra-CU handover).
  void set_config(const ue_location_manager_cfg& new_cfg) { cfg = new_cfg; }

  /// \brief Extract the current location reporting state for transfer.
  ue_location_manager_cfg get_config() const { return cfg; }

  /// \brief Store a location reporting configuration received from AMF.
  /// \returns nullopt on success, or a Location Reporting Failure Indication cause if the configuration failed.
  std::optional<ngap_cause_t> configure_location_reporting(const ngap_location_report_request& ctrl);

  /// \brief Build and return a location report, if location reporting is configured.
  std::optional<ngap_location_report> get_location_report(ue_index_t                         ue_index,
                                                          const cu_cp_user_location_info_nr& user_location_info);

  /// \brief Build and return a direct location report, using the provided request.
  ngap_location_report get_direct_location_report(ue_index_t                          ue_index,
                                                  const cu_cp_user_location_info_nr&  user_location_info,
                                                  const ngap_location_report_request& request);

private:
  ngap_location_report_request::event_type get_current_location_reporting_type() const;

  static ngap_ue_presence check_ue_presence(const ngap_area_of_interest& aoi, const cu_cp_user_location_info_nr& loc);

  ue_location_manager_cfg cfg;
  ocudulog::basic_logger& logger;
};

} // namespace ocudu::ocucp
