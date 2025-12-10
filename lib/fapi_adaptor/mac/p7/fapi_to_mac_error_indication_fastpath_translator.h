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

#include "ocudu/fapi/common/error_indication_notifier.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {

class mac_cell_slot_handler;

namespace fapi_adaptor {

/// FAPI-to-MAC error indication fastpath translator.
class fapi_to_mac_error_indication_fastpath_translator : public fapi::error_indication_notifier
{
public:
  explicit fapi_to_mac_error_indication_fastpath_translator(ocudulog::basic_logger& logger_);

  // See interface for documentation.
  void on_error_indication(const fapi::error_indication& msg) override;

  /// Sets the given cell-specific slot handler. This handler will be used to receive error notifications.
  void set_cell_slot_handler(mac_cell_slot_handler& handler) { mac_slot_handler = &handler; }

private:
  mac_cell_slot_handler*  mac_slot_handler;
  ocudulog::basic_logger& logger;
};

} // namespace fapi_adaptor
} // namespace ocudu
