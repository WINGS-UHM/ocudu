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

#include "ocudu/fapi/p7/messages/uci_pusch_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// UCI PUSCH PDU builder that helps fill in the parameters specified in SCF-222 v4.0 section 3.4.9.1.
class uci_pusch_pdu_builder
{
  uci_pusch_pdu& pdu;

public:
  explicit uci_pusch_pdu_builder(uci_pusch_pdu& pdu_) : pdu(pdu_) { pdu.pdu_bitmap = 0; }

  /// \brief Sets the UCI PUSCH PDU basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.1 in Table UCI PUSCH PDU.
  uci_pusch_pdu_builder& set_basic_parameters(uint32_t handle, rnti_t rnti)
  {
    pdu.handle = handle;
    pdu.rnti   = to_value(rnti);

    return *this;
  }

  /// \brief Sets the UCI PUSCH PDU metrics parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.1 in Table UCI PUSCH PDU.
  uci_pusch_pdu_builder& set_metrics_parameters(std::optional<float>    ul_sinr_metric,
                                                std::optional<unsigned> timing_advance_offset,
                                                std::optional<int>      timing_advance_offset_ns,
                                                std::optional<float>    rssi,
                                                std::optional<float>    rsrp,
                                                bool                    rsrp_use_dBm = false)
  {
    pdu.timing_advance_offset    = (timing_advance_offset) ? static_cast<uint16_t>(timing_advance_offset.value())
                                                           : std::numeric_limits<uint16_t>::max();
    pdu.timing_advance_offset_ns = (timing_advance_offset_ns) ? static_cast<int16_t>(timing_advance_offset_ns.value())
                                                              : std::numeric_limits<int16_t>::min();

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

  /// \brief Sets the HARQ PDU parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.4 in Table HARQ PDU for Format 2, Format 3 or
  /// Format 4 or for PUSCH.
  uci_pusch_pdu_builder& set_harq_parameters(uci_pusch_or_pucch_f2_3_4_detection_status detection,
                                             uint16_t                                   expected_bit_length,
                                             const bounded_bitset<uci_constants::MAX_NOF_HARQ_BITS>& payload)
  {
    pdu.pdu_bitmap.set(uci_pusch_pdu::HARQ_BIT);

    auto& harq               = pdu.harq;
    harq.detection_status    = detection;
    harq.expected_bit_length = expected_bit_length;
    harq.payload             = payload;

    return *this;
  }

  /// \brief Sets the CSI Part 1 PDU parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.4 in Table CSI Part 1 PDU.
  uci_pusch_pdu_builder&
  set_csi_part1_parameters(uci_pusch_or_pucch_f2_3_4_detection_status                            detection,
                           uint16_t                                                              expected_bit_length,
                           const bounded_bitset<uci_constants::MAX_NOF_CSI_PART1_OR_PART2_BITS>& payload)
  {
    pdu.pdu_bitmap.set(uci_pusch_pdu::CSI_PART1_BIT);

    auto& csi               = pdu.csi_part1;
    csi.detection_status    = detection;
    csi.expected_bit_length = expected_bit_length;
    csi.payload             = payload;

    return *this;
  }

  /// \brief Sets the CSI Part 2 PDU parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.9.4 in Table CSI Part 2 PDU.
  uci_pusch_pdu_builder&
  set_csi_part2_parameters(uci_pusch_or_pucch_f2_3_4_detection_status                            detection,
                           uint16_t                                                              expected_bit_length,
                           const bounded_bitset<uci_constants::MAX_NOF_CSI_PART1_OR_PART2_BITS>& payload)
  {
    pdu.pdu_bitmap.set(uci_pusch_pdu::CSI_PART2_BIT);

    auto& csi               = pdu.csi_part2;
    csi.detection_status    = detection;
    csi.expected_bit_length = expected_bit_length;
    csi.payload             = payload;

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
