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

#include "ocudu/fapi/p7/messages/rach_indication.h"
#include <optional>

namespace ocudu {
namespace fapi {

/// RACH.indication PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.11.
class rach_indication_pdu_builder
{
  rach_indication_pdu& pdu;

public:
  explicit rach_indication_pdu_builder(rach_indication_pdu& pdu_) : pdu(pdu_) {}

  /// \brief Sets the time domain parameters of the \e RACH.indication PDU and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_pdu_builder& set_time_domain_parameters(uint8_t symbol_index, uint8_t slot_index)

  {
    pdu.symbol_index = symbol_index;
    pdu.slot_index   = slot_index;

    return *this;
  }

  /// \brief Sets the frequency domain parameters of the \e RACH.indication PDU and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_pdu_builder& set_frequency_domain_parameters(uint8_t ra_index)
  {
    pdu.ra_index = ra_index;

    return *this;
  }

  /// \brief Sets the power parameters of the \e RACH.indication PDU and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_pdu_builder& set_power_parameters(std::optional<float> avg_rssi_dB, std::optional<float> avg_snr_dB)

  {
    pdu.avg_rssi = (avg_rssi_dB) ? static_cast<uint32_t>((avg_rssi_dB.value() + 140.F) * 1000.F)
                                 : std::numeric_limits<uint32_t>::max();

    unsigned avg_snr =
        (avg_snr_dB) ? static_cast<unsigned>((avg_snr_dB.value() + 64.F) * 2) : std::numeric_limits<uint8_t>::max();

    ocudu_assert(avg_snr <= std::numeric_limits<uint8_t>::max(),
                 "Average SNR ({}) exceeds the maximum ({}).",
                 avg_snr,
                 std::numeric_limits<uint8_t>::max());
    pdu.avg_snr = static_cast<uint8_t>(avg_snr);

    return *this;
  }

  /// \brief Adds a preamble to the \e RACH.indication PDU and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  /// Units for timing advace offset parameter are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication
  /// message body, and this function expect this units.
  rach_indication_pdu_builder& add_preamble(unsigned                     preamble_index,
                                            std::optional<phy_time_unit> timing_advance_offset,
                                            std::optional<float>         preamble_power,
                                            std::optional<float>         preamble_snr)

  {
    auto& preamble = pdu.preambles.emplace_back();

    preamble.preamble_index = preamble_index;

    preamble.timing_advance_offset = timing_advance_offset;

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
class rach_indication_builder
{
  rach_indication& msg;

public:
  explicit rach_indication_builder(rach_indication& msg_) : msg(msg_) {}

  /// \brief Sets the slot point of the \e RACH.indication message and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_builder& set_slot_point(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// \brief Adds a PDU to the \e RACH.indication message and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 section 3.4.11 in table RACH.indication message body.
  rach_indication_pdu_builder add_pdu(uint8_t              symbol_index,
                                      uint8_t              slot_index,
                                      uint8_t              ra_index,
                                      std::optional<float> avg_rssi,
                                      std::optional<float> avg_snr)
  {
    auto& pdu = msg.pdus.emplace_back();

    rach_indication_pdu_builder builder(pdu);

    builder.set_time_domain_parameters(symbol_index, slot_index)
        .set_frequency_domain_parameters(ra_index)
        .set_power_parameters(avg_rssi, avg_snr);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
