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

#include "ocudu/fapi/p7/messages/rach_indication.h"
#include <optional>

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// RACH.indication PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.11.
class rach_indication_pdu_builder
{
  rach_indication_pdu& pdu;

public:
  explicit rach_indication_pdu_builder(rach_indication_pdu& pdu_) : pdu(pdu_) {}

  /// Sets the basic parameters of the RACH.indication PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_pdu_builder& set_basic_params(uint16_t             handle,
                                                uint8_t              symbol_index,
                                                uint8_t              slot_index,
                                                uint8_t              ra_index,
                                                std::optional<float> avg_rssi_dB,
                                                std::optional<float> rsrp,
                                                std::optional<float> avg_snr_dB,
                                                bool                 rsrp_use_dBm = false)

  {
    pdu.handle       = handle;
    pdu.symbol_index = symbol_index;
    pdu.slot_index   = slot_index;
    pdu.ra_index     = ra_index;

    pdu.avg_rssi = (avg_rssi_dB) ? static_cast<uint32_t>((avg_rssi_dB.value() + 140.F) * 1000.F)
                                 : std::numeric_limits<uint32_t>::max();

    unsigned avg_snr =
        (avg_snr_dB) ? static_cast<unsigned>((avg_snr_dB.value() + 64.F) * 2) : std::numeric_limits<uint8_t>::max();

    ocudu_assert(avg_snr <= std::numeric_limits<uint8_t>::max(),
                 "Average SNR ({}) exceeds the maximum ({}).",
                 avg_snr,
                 std::numeric_limits<uint8_t>::max());
    pdu.avg_snr = static_cast<uint8_t>(avg_snr);

    unsigned rsrp_value = (rsrp) ? static_cast<unsigned>((rsrp.value() + ((rsrp_use_dBm) ? 140.F : 128.F)) * 10.F)
                                 : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rsrp_value <= std::numeric_limits<uint16_t>::max(),
                 "RSRP ({}) exceeds the maximum ({}).",
                 rsrp_value,
                 std::numeric_limits<uint16_t>::max());

    pdu.rsrp = static_cast<uint16_t>(rsrp_value);

    return *this;
  }

  /// Adds a preamble to the RACH.indication PDU and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  /// \note Units for timing advace offset parameter are specified in SCF-222 v4.0 section 3.4.11 in table
  /// RACH.indication message body, and this function expect this units.
  rach_indication_pdu_builder& add_preamble(unsigned                preamble_index,
                                            std::optional<unsigned> timing_advance_offset,
                                            std::optional<uint32_t> timing_advance_offset_ns,
                                            std::optional<float>    preamble_power,
                                            std::optional<float>    preamble_snr)

  {
    auto& preamble = pdu.preambles.emplace_back();

    preamble.preamble_index = preamble_index;

    preamble.timing_advance_offset =
        (timing_advance_offset) ? timing_advance_offset.value() : std::numeric_limits<uint16_t>::max();

    preamble.timing_advance_offset_ns =
        (timing_advance_offset_ns) ? timing_advance_offset_ns.value() : std::numeric_limits<uint32_t>::max();

    preamble.preamble_pwr = (preamble_power) ? static_cast<uint32_t>((preamble_power.value() + 140.F) * 1000.F)
                                             : std::numeric_limits<uint32_t>::max();

    unsigned snr = (preamble_snr) ? static_cast<unsigned>((preamble_snr.value() + 64.F) * 2.F)
                                  : std::numeric_limits<uint8_t>::max();

    ocudu_assert(snr <= std::numeric_limits<uint8_t>::max(),
                 "Preamble SNR ({}) exceeds the maximum ({}).",
                 snr,
                 std::numeric_limits<uint8_t>::max());

    preamble.preamble_snr = static_cast<uint8_t>(snr);

    return *this;
  }
};

/// \e RACH.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 Section 3.4.11.
class rach_indication_message_builder
{
  rach_indication_message& msg;

public:
  explicit rach_indication_message_builder(rach_indication_message& msg_) : msg(msg_) {}

  /// Sets the basic parameters of the RACH.indication message and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_message_builder& set_basic_parameters(uint16_t sfn, uint16_t slot)
  {
    msg.sfn  = sfn;
    msg.slot = slot;

    return *this;
  }

  /// Adds a PDU to the RACH.indication message and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_pdu_builder add_pdu(uint16_t             handle,
                                      uint8_t              symbol_index,
                                      uint8_t              slot_index,
                                      uint8_t              ra_index,
                                      std::optional<float> avg_rssi,
                                      std::optional<float> rsrp,
                                      std::optional<float> avg_snr,
                                      bool                 rsrp_use_dBm = false)
  {
    auto& pdu = msg.pdus.emplace_back();

    rach_indication_pdu_builder builder(pdu);

    builder.set_basic_params(handle, symbol_index, slot_index, ra_index, avg_rssi, rsrp, avg_snr, rsrp_use_dBm);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
