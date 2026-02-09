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

#include "ocudu/adt/span.h"
#include "ocudu/fapi/p7/messages/rx_data_indication.h"
#include "ocudu/ran/slot_point.h"

namespace ocudu {
namespace fapi {

/// Rx_Data.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.7.
class rx_data_indication_builder
{
  rx_data_indication& msg;

public:
  explicit rx_data_indication_builder(rx_data_indication& msg_) : msg(msg_) {}

  /// \brief Sets the \e Rx_Data.indication slot point and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.7 in table Rx_Data.indication message body.
  rx_data_indication_builder& set_slot_point(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// \brief Adds a PDU to the message and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.7 in table Rx_Data.indication message body.
  rx_data_indication_builder& add_pdu(rnti_t rnti, harq_id_t harq_id, span<const uint8_t> transport_block)
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.rnti            = rnti;
    pdu.harq_id         = harq_id;
    pdu.transport_block = transport_block;

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
