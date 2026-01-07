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

/// Dummy P7 indications notifier implementation that will terminate the application if its methods are called.
class p7_indications_notifier_dummy : public p7_indications_notifier
{
public:
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
};

} // namespace fapi
} // namespace ocudu
