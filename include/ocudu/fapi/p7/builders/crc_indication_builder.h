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
#include "ocudu/fapi/p7/messages/crc_indication.h"

namespace ocudu {
namespace fapi {

/// CRC.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.8.
class crc_indication_builder
{
  crc_indication& msg;

public:
  explicit crc_indication_builder(crc_indication& msg_) : msg(msg_) {}

  /// Sets the \e CRC.indication basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.8 in table CRC.indication message body.
  crc_indication_builder& set_basic_parameters(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// Adds a \e CRC.indication PDU to the message and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.8 in table CRC.indication message body.
  crc_indication_builder& add_pdu(rnti_t                       rnti,
                                  harq_id_t                    harq_id,
                                  bool                         tb_crc_status_ok,
                                  std::optional<float>         ul_sinr_dB,
                                  std::optional<phy_time_unit> timing_advance_offset,
                                  std::optional<float>         rssi_dB,
                                  std::optional<float>         rsrp,
                                  bool                         rsrp_use_dBm = false)
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.rnti                  = rnti;
    pdu.harq_id               = harq_id;
    pdu.tb_crc_status_ok      = tb_crc_status_ok;
    pdu.timing_advance_offset = timing_advance_offset;

    unsigned rssi =
        (rssi_dB) ? static_cast<unsigned>((rssi_dB.value() + 128.F) * 10.F) : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rssi <= std::numeric_limits<uint16_t>::max(),
                 "RSSI ({}) exceeds the maximum ({}).",
                 rssi,
                 std::numeric_limits<uint16_t>::max());

    pdu.rssi = static_cast<uint16_t>(rssi);

    unsigned rsrp_value = (rsrp) ? static_cast<unsigned>((rsrp.value() + ((rsrp_use_dBm) ? 140.F : 128.F)) * 10.F)
                                 : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rsrp_value <= std::numeric_limits<uint16_t>::max(),
                 "RSRP ({}) exceeds the maximum ({}).",
                 rsrp_value,
                 std::numeric_limits<uint16_t>::max());

    pdu.rsrp = static_cast<uint16_t>(rsrp_value);

    int ul_sinr = (ul_sinr_dB) ? static_cast<int>(ul_sinr_dB.value() * 500.F) : std::numeric_limits<int16_t>::min();

    ocudu_assert(ul_sinr <= std::numeric_limits<int16_t>::max(),
                 "UL SINR metric ({}) exceeds the maximum ({}).",
                 ul_sinr,
                 std::numeric_limits<int16_t>::max());

    ocudu_assert(ul_sinr >= std::numeric_limits<int16_t>::min(),
                 "UL SINR metric ({}) is under the minimum ({}).",
                 ul_sinr,
                 std::numeric_limits<int16_t>::min());

    pdu.ul_sinr_metric = static_cast<int16_t>(ul_sinr);
    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
