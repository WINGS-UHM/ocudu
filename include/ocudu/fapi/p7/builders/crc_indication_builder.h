/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
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

// :TODO: Review the builders documentation so it matches the UCI builder.

/// CRC.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.8.
class crc_indication_builder
{
  crc_indication& msg;

public:
  explicit crc_indication_builder(crc_indication& msg_) : msg(msg_) {}

  /// Sets the CRC.indication basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.8 in table CRC.indication message body.
  crc_indication_builder& set_basic_parameters(uint16_t sfn, uint16_t slot)
  {
    msg.sfn  = sfn;
    msg.slot = slot;

    return *this;
  }

  /// Adds a CRC.indication PDU to the message and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.8 in table CRC.indication message body.
  crc_indication_builder& add_pdu(uint32_t                handle,
                                  rnti_t                  rnti,
                                  std::optional<uint8_t>  rapid,
                                  uint8_t                 harq_id,
                                  bool                    tb_crc_status_ok,
                                  uint16_t                num_cb,
                                  span<const uint8_t>     cb_crc_status,
                                  std::optional<float>    ul_sinr_dB,
                                  std::optional<unsigned> timing_advance_offset,
                                  std::optional<int>      timing_advance_offset_in_ns,
                                  std::optional<float>    rssi_dB,
                                  std::optional<float>    rsrp,
                                  bool                    rsrp_use_dBm = false)
  {
    auto& pdu = msg.pdus.emplace_back();

    pdu.handle           = handle;
    pdu.rnti             = rnti;
    pdu.rapid            = (rapid) ? rapid.value() : 255U;
    pdu.harq_id          = harq_id;
    pdu.tb_crc_status_ok = tb_crc_status_ok;
    pdu.num_cb           = num_cb;
    pdu.cb_crc_status.assign(cb_crc_status.begin(), cb_crc_status.end());
    pdu.timing_advance_offset =
        (timing_advance_offset) ? timing_advance_offset.value() : std::numeric_limits<uint16_t>::max();
    pdu.timing_advance_offset_ns =
        (timing_advance_offset_in_ns) ? timing_advance_offset_in_ns.value() : std::numeric_limits<int16_t>::min();

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
