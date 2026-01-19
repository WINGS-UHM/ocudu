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
#include "ocudu/fapi/p7/messages/uci_pucch_pdu_format_0_1.h"
#include "ocudu/ran/pucch/pucch_mapping.h"
#include "ocudu/ran/rnti.h"
#include <optional>

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// UCI PUCCH PDU Format 0 or Format 1 builder that helps fill in the parameters specified in SCF-222 v4.0
/// Section 3.4.9.2.
class uci_pucch_pdu_format_0_1_builder
{
  uci_pucch_pdu_format_0_1& pdu;

public:
  explicit uci_pucch_pdu_format_0_1_builder(uci_pucch_pdu_format_0_1& pdu_) : pdu(pdu_) { pdu.pdu_bitmap = 0; }

  /// \brief Sets the UCI PUCCH PDU Format 0 and Format 1 basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.2 in Table UCI PUCCH Format 0 or Format 1 PDU.
  uci_pucch_pdu_format_0_1_builder& set_basic_parameters(rnti_t rnti, pucch_format type)
  {
    pdu.rnti = to_value(rnti);
    switch (type) {
      case pucch_format::FORMAT_0:
        pdu.pucch_format = uci_pucch_pdu_format_0_1::format_type::format_0;
        break;
      case pucch_format::FORMAT_1:
        pdu.pucch_format = uci_pucch_pdu_format_0_1::format_type::format_1;
        break;
      default:
        ocudu_assert(0, "PUCCH format={} is not supported by this PDU", fmt::underlying(type));
        break;
    }

    return *this;
  }

  /// \brief Sets the UCI PUCCH PDU Format 0 and Format 1 metrics parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.2 in Table UCI PUCCH Format 0 or Format 1 PDU.
  uci_pucch_pdu_format_0_1_builder& set_metrics_parameters(std::optional<float>         ul_sinr_metric,
                                                           std::optional<phy_time_unit> timing_advance_offset,
                                                           std::optional<float>         rssi,
                                                           std::optional<float>         rsrp,
                                                           bool                         rsrp_use_dBm = false)
  {
    pdu.timing_advance_offset = timing_advance_offset;

    // SINR.
    int sinr =
        (ul_sinr_metric) ? static_cast<int>(ul_sinr_metric.value() * 500.F) : std::numeric_limits<int16_t>::min();

    ocudu_assert(sinr <= std::numeric_limits<int16_t>::max(),
                 "UL SINR metric ({}) exceeds the maximum ({}).",
                 sinr,
                 std::numeric_limits<int16_t>::max());

    ocudu_assert(sinr >= std::numeric_limits<int16_t>::min(),
                 "UL SINR metric ({}) is under the minimum ({}).",
                 sinr,
                 std::numeric_limits<int16_t>::min());

    pdu.ul_sinr_metric = static_cast<int16_t>(sinr);

    // RSSI.
    unsigned rssi_value =
        (rssi) ? static_cast<unsigned>((rssi.value() + 128.F) * 10.F) : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rssi_value <= std::numeric_limits<uint16_t>::max(),
                 "RSSI metric ({}) exceeds the maximum ({}).",
                 rssi_value,
                 std::numeric_limits<uint16_t>::max());

    pdu.rssi = static_cast<uint16_t>(rssi_value);

    // RSRP.
    unsigned rsrp_value = (rsrp) ? static_cast<unsigned>((rsrp.value() + ((rsrp_use_dBm) ? 140.F : 128.F)) * 10.F)
                                 : std::numeric_limits<uint16_t>::max();

    ocudu_assert(rsrp_value <= std::numeric_limits<uint16_t>::max(),
                 "RSRP metric ({}) exceeds the maximum ({}).",
                 rsrp_value,
                 std::numeric_limits<uint16_t>::max());

    pdu.rsrp = static_cast<uint16_t>(rsrp_value);

    return *this;
  }

  /// \brief Sets the SR PDU parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.4 in Table SR PDU for Format 0 or Format 1 PDU.
  uci_pucch_pdu_format_0_1_builder& set_sr_parameters(bool detected, std::optional<unsigned> confidence_level)
  {
    pdu.pdu_bitmap.set(uci_pucch_pdu_format_0_1::SR_BIT);

    auto& sr_pdu = pdu.sr;

    sr_pdu.sr_indication = detected;

    sr_pdu.sr_confidence_level = (confidence_level) ? confidence_level.value() : std::numeric_limits<uint8_t>::max();

    return *this;
  }

  /// \brief Sets the HARQ PDU parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.2 in Table HARQ PDU for Format 0 or Format 1
  /// PDU.
  uci_pucch_pdu_format_0_1_builder& set_harq_parameters(std::optional<unsigned>                    confidence_level,
                                                        span<const uci_pucch_f0_or_f1_harq_values> value)
  {
    pdu.pdu_bitmap.set(uci_pucch_pdu_format_0_1::HARQ_BIT);

    auto& harq                 = pdu.harq;
    harq.harq_confidence_level = (confidence_level) ? confidence_level.value() : std::numeric_limits<uint8_t>::max();
    harq.harq_values.assign(value.begin(), value.end());

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
