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

#include "ocudu/fapi/p7/p7_indications_notifier.h"

namespace ocudu {
namespace fapi {

/// Slot data message notifier dispatcher that forwards data messages to the configured notifier.
class p7_indications_notifier_dispatcher : public p7_indications_notifier
{
  p7_indications_notifier* notifier = nullptr;

public:
  p7_indications_notifier_dispatcher();

  // See interface for documentation.
  void on_rx_data_indication(const rx_data_indication& msg) override;

  // See interface for documentation.
  void on_crc_indication(const crc_indication& msg) override;

  // See interface for documentation.
  void on_uci_indication(const uci_indication& msg) override;

  // See interface for documentation.
  void on_srs_indication(const srs_indication& msg) override;

  // See interface for documentation.
  void on_rach_indication(const rach_indication& msg) override;

  /// Sets the P7 indications notifier to the given one.
  void set_p7_indications_notifier(p7_indications_notifier& p7_notifier);
};

} // namespace fapi
} // namespace ocudu
