/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ocudu/rrc/meas_types_validators.h"
#include "ocudu/ocudulog/ocudulog.h"

using namespace ocudu;
using namespace ocucp;

#define LOG_CHAN ("RRC")

bool ocudu::ocucp::validate_config(const rrc_meas_trigger_quant_offset& config)
{
  if (not config.rsrp.has_value() and not config.rsrq.has_value() and not config.sinr.has_value()) {
    ocudulog::fetch_basic_logger(LOG_CHAN).error("Either RSRP/RSRQ or SINR need to be configured.");
    return false;
  }
  return true;
}

bool ocudu::ocucp::validate_config(const rrc_cond_event_a3& event)
{
  if (not validate_config(event.a3_offset)) {
    ocudulog::fetch_basic_logger(LOG_CHAN).error("A3 offset config not valid.");
    return false;
  }

  // TODO: add range checks for hysteresis and time to trigger

  return true;
}
