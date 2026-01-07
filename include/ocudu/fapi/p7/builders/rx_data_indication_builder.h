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

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// Rx_Data.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.7.
class rx_data_indication_builder
{
  rx_data_indication& msg;

public:
  explicit rx_data_indication_builder(rx_data_indication& msg_) : msg(msg_) {}

  /// Sets the Rx_Data.indication basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.7 in table Rx_Data.indication message body.
  rx_data_indication_builder& set_basic_parameters(uint16_t sfn, uint16_t slot, uint16_t control_length)
  {
    msg.sfn            = sfn;
    msg.slot           = slot;
    msg.control_length = control_length;

    return *this;
  }

  /// Adds a PDU to the message and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.7 in table Rx_Data.indication message body.
  rx_data_indication_builder&
  add_custom_pdu(uint32_t handle, rnti_t rnti, std::optional<unsigned> rapid, uint8_t harq_id, span<const uint8_t> data)
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.handle  = handle;
    pdu.rnti    = rnti;
    pdu.rapid   = (rapid) ? static_cast<uint8_t>(rapid.value()) : 255U;
    pdu.harq_id = harq_id;

    // Mark the PDU as custom. This part of the message is not compliant with FAPI.
    pdu.pdu_tag    = rx_data_indication_pdu::pdu_tag_type::custom;
    pdu.pdu_length = data.size();
    pdu.data       = data.data();

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
