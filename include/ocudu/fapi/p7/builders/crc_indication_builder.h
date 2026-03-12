// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/p7/messages/crc_indication.h"

namespace ocudu {
namespace fapi {

/// CRC.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.8.
class crc_indication_builder
{
  crc_indication& msg;

public:
  explicit crc_indication_builder(crc_indication& msg_) : msg(msg_) {}

  /// \brief Sets the \e CRC.indication slot and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.8 in table CRC.indication message body.
  crc_indication_builder& set_slot(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// \brief Sets a \e CRC.indication PDU to the message and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.8 in table CRC.indication message body.
  crc_indication_builder& set_pdu(rnti_t                       rnti,
                                  harq_id_t                    harq_id,
                                  bool                         tb_crc_status_ok,
                                  std::optional<float>         ul_sinr_dB,
                                  std::optional<phy_time_unit> timing_advance_offset,
                                  std::optional<float>         rssi_dB,
                                  std::optional<float>         rsrp,
                                  bool                         rsrp_use_dBm = false)
  {
    msg.pdu.rnti                  = rnti;
    msg.pdu.harq_id               = harq_id;
    msg.pdu.tb_crc_status_ok      = tb_crc_status_ok;
    msg.pdu.timing_advance_offset = timing_advance_offset;

    unsigned rssi =
        (rssi_dB) ? static_cast<unsigned>((rssi_dB.value() + 128.F) * 10.F) : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rssi <= std::numeric_limits<uint16_t>::max(),
                 "RSSI ({}) exceeds the maximum ({}).",
                 rssi,
                 std::numeric_limits<uint16_t>::max());

    msg.pdu.rssi = static_cast<uint16_t>(rssi);

    unsigned rsrp_value = (rsrp) ? static_cast<unsigned>((rsrp.value() + ((rsrp_use_dBm) ? 140.F : 128.F)) * 10.F)
                                 : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rsrp_value <= std::numeric_limits<uint16_t>::max(),
                 "RSRP ({}) exceeds the maximum ({}).",
                 rsrp_value,
                 std::numeric_limits<uint16_t>::max());

    msg.pdu.rsrp = static_cast<uint16_t>(rsrp_value);

    int ul_sinr = (ul_sinr_dB) ? static_cast<int>(ul_sinr_dB.value() * 500.F) : std::numeric_limits<int16_t>::min();

    ocudu_assert(ul_sinr <= std::numeric_limits<int16_t>::max(),
                 "UL SINR metric ({}) exceeds the maximum ({}).",
                 ul_sinr,
                 std::numeric_limits<int16_t>::max());

    ocudu_assert(ul_sinr >= std::numeric_limits<int16_t>::min(),
                 "UL SINR metric ({}) is under the minimum ({}).",
                 ul_sinr,
                 std::numeric_limits<int16_t>::min());

    msg.pdu.ul_sinr_metric = static_cast<int16_t>(ul_sinr);
    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
