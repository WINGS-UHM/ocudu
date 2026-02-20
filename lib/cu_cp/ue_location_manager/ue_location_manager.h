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
#include <optional>

namespace ocudu::ocucp {

class ue_location_manager
{
public:
  ue_location_manager();

  /// \brief Store a location reporting configuration received from AMF.
  void configure_location_reporting(const ngap_location_report_request& ctrl);

  /// \brief Build and return a location.
  ngap_location_report get_location_report(ue_index_t ue_index, const nr_cell_global_id_t& nr_cgi, const tai_t& tai);

private:
  std::optional<ngap_location_report_request> active_config;
  bool                                        direct_report_pending = false;

  ocudulog::basic_logger& logger;
};

} // namespace ocudu::ocucp
