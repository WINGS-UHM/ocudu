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
#include "ocudu/ocudulog/ocudulog.h"

namespace ocudu {
namespace fapi {

/// Adds logging information over the implemented interface.
class logging_p7_indications_notifier_decorator : public p7_indications_notifier
{
public:
  logging_p7_indications_notifier_decorator(unsigned sector_id_, ocudulog::basic_logger& logger_);

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

private:
  const unsigned           sector_id;
  ocudulog::basic_logger&  logger;
  p7_indications_notifier* notifier;
};

} // namespace fapi
} // namespace ocudu
