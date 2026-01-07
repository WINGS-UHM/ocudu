/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "p7_indications_notifier_dummy.h"
#include "ocudu/support/error_handling.h"

using namespace ocudu;
using namespace fapi;

void p7_indications_notifier_dummy::on_rx_data_indication(const rx_data_indication& msg)
{
  report_error("Dummy FAPI slot data message notifier cannot handle given rx data indication");
}

void p7_indications_notifier_dummy::on_crc_indication(const crc_indication& msg)
{
  report_error("Dummy FAPI slot data message notifier cannot handle given CRC indication");
}

void p7_indications_notifier_dummy::on_uci_indication(const uci_indication& msg)
{
  report_error("Dummy FAPI slot data message notifier cannot handle given UCI indication");
}

void p7_indications_notifier_dummy::on_srs_indication(const srs_indication& msg)
{
  report_error("Dummy FAPI slot data message notifier cannot handle given SRS indication");
}

void p7_indications_notifier_dummy::on_rach_indication(const rach_indication& msg)
{
  report_error("Dummy FAPI slot data message notifier cannot handle given RACH indication");
}
