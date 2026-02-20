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
  active_config = ctrl;

  if (ctrl.location_reporting_type == ngap_location_report_request::event_type::direct) {
    direct_report_pending = true;
  } else {
    logger.warning("Unsupported location reporting type: {}", ctrl.location_reporting_type);
  }
}

ngap_location_report
ue_location_manager::get_location_report(ue_index_t ue_index, const nr_cell_global_id_t& nr_cgi, const tai_t& tai)
{
  ngap_location_report report;
  report.ue_index = ue_index;
  report.nr_cgi   = nr_cgi;
  report.tai      = tai;
  return report;
}
