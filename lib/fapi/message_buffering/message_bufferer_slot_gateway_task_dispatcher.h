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

#include "message_bufferer_slot_gateway_impl.h"
#include "ocudu/fapi/p7/p7_slot_indication_notifier.h"

namespace ocudu {

class task_executor;

namespace fapi {

/// Buffered slot gateway task dispatcher.
class message_bufferer_slot_gateway_task_dispatcher : public p7_requests_gateway
{
public:
  message_bufferer_slot_gateway_task_dispatcher(unsigned             sector_id_,
                                                unsigned             l2_nof_slots_ahead,
                                                subcarrier_spacing   scs_,
                                                p7_requests_gateway& gateway,
                                                task_executor&       executor_);

  /// Updates the current slot of the message bufferer slot gateway.
  void update_current_slot(slot_point slot);

  /// Forwards cached messages for the given slot.
  void forward_cached_messages(slot_point slot);

  // See interface for documentation.
  void send_dl_tti_request(const dl_tti_request& msg) override;

  // See interface for documentation.
  void send_ul_tti_request(const ul_tti_request& msg) override;

  // See interface for documentation.
  void send_ul_dci_request(const ul_dci_request& msg) override;

  // See interface for documentation.
  void send_tx_data_request(const tx_data_request& msg) override;

private:
  const unsigned                     sector_id;
  ocudulog::basic_logger&            logger;
  task_executor&                     executor;
  message_bufferer_slot_gateway_impl message_bufferer_gateway;
};

} // namespace fapi
} // namespace ocudu
