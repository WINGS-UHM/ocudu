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
#include "ocudu/ocudulog/logger.h"
#include "ocudu/phy/upper/upper_phy_rx_results_notifier.h"

namespace ocudu {
namespace fapi_adaptor {

/// \brief PHY-to-FAPI uplink results events fastpath translator.
///
/// This class listens to upper PHY uplink result events and translates them into FAPI indication messages that are sent
/// through the FAPI data-specific message notifier.
class phy_to_fapi_results_event_fastpath_translator : public upper_phy_rx_results_notifier
{
public:
  phy_to_fapi_results_event_fastpath_translator(unsigned                sector_id_,
                                                float                   dBFS_calibration_value_,
                                                ocudulog::basic_logger& logger_);

  // See interface for documentation.
  void on_new_prach_results(const ul_prach_results& result) override;

  // See interface for documentation.
  void on_new_pusch_results_control(const ul_pusch_results_control& result) override;

  // See interface for documentation.
  void on_new_pusch_results_data(const ul_pusch_results_data& result) override;

  // See interface for documentation.
  void on_new_pucch_results(const ul_pucch_results& result) override;

  // See interface for documentation.
  void on_new_srs_results(const ul_srs_results& result) override;

  /// Configures the FAPI P7 indications notifier to the given one.
  void set_p7_indications_notifier(fapi::p7_indications_notifier& fapi_p7_notifier) { p7_notifier = &fapi_p7_notifier; }

private:
  /// Notifies a new FAPI \e CRC.indication through the data notifier.
  void notify_crc_indication(const ul_pusch_results_data& result);

  /// Notifies a new FAPI \e Rx_Data.indication through the data notifier.
  void notify_rx_data_indication(const ul_pusch_results_data& result);

  ///  Notifies a new FAPI \e UCI.indication through the data notifier that carries a PUSCH PDU.
  void notify_pusch_uci_indication(const ul_pusch_results_control& result);

private:
  /// Radio sector identifier.
  const unsigned sector_id;
  /// dBFS calibration value.
  const float dBFS_calibration_value;
  /// FAPI logger.
  ocudulog::basic_logger& logger;
  /// FAPI P7 indications notifier.
  fapi::p7_indications_notifier* p7_notifier;
};

} // namespace fapi_adaptor
} // namespace ocudu
