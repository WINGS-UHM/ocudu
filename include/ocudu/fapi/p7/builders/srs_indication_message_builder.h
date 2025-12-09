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

#include "ocudu/fapi/p7/messages/srs_indication.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// SRS indication PDU builder that helps fill in the parameters specified in SCF-222 v4.0 section 3.4.10.
class srs_indication_pdu_builder
{
  srs_indication_pdu& pdu;

public:
  explicit srs_indication_pdu_builder(srs_indication_pdu& pdu_) : pdu(pdu_) {}

  /// \brief Sets the SRS indication PDU basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.10.
  srs_indication_pdu_builder& set_basic_parameters(uint32_t handle, rnti_t rnti)
  {
    pdu.handle = handle;
    pdu.rnti   = rnti;

    return *this;
  }

  /// \brief Sets the SRS indication PDU metrics parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.10.
  srs_indication_pdu_builder& set_metrics_parameters(std::optional<unsigned> timing_advance_offset,
                                                     std::optional<int>      timing_advance_offset_ns)
  {
    pdu.timing_advance_offset    = (timing_advance_offset) ? static_cast<uint16_t>(timing_advance_offset.value())
                                                           : std::numeric_limits<uint16_t>::max();
    pdu.timing_advance_offset_ns = (timing_advance_offset_ns) ? static_cast<int16_t>(timing_advance_offset_ns.value())
                                                              : std::numeric_limits<int16_t>::min();

    return *this;
  }

  /// \brief Sets the SRS indication PDU normalized channel I/Q matrix and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.10 Table 3-132.
  srs_indication_pdu_builder& set_codebook_report_matrix(const srs_channel_matrix& matrix)
  {
    pdu.usage       = srs_usage::codebook;
    pdu.report_type = srs_report_type::normalized_channel_iq_matrix;
    pdu.matrix      = matrix;

    return *this;
  }

  /// \brief Sets the SRS indication PDU positioning report and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v8.0 Section 3.4.10 Table 3-209.
  srs_indication_pdu_builder& set_positioning_report_parameters(std::optional<phy_time_unit> ul_relative_toa,
                                                                std::optional<uint32_t>      gnb_rx_tx_difference,
                                                                std::optional<uint16_t>      ul_aoa,
                                                                std::optional<float>         rsrp)
  {
    pdu.report_type                      = srs_report_type::positioning;
    pdu.positioning.ul_relative_toa      = ul_relative_toa;
    pdu.positioning.gnb_rx_tx_difference = gnb_rx_tx_difference;
    pdu.positioning.ul_aoa               = ul_aoa;
    pdu.positioning.rsrp                 = rsrp;
    // [Implementation defined] Set this one to local as it is not used.
    pdu.positioning.coordinate_system_aoa = srs_coordinate_system_ul_aoa::local;
    // [Implementation defined] Set the usage to a enum value.
    pdu.usage = srs_usage::codebook;

    return *this;
  }
};

/// SRS.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 Section 3.4.10.
class srs_indication_message_builder
{
  srs_indication_message& msg;

public:
  explicit srs_indication_message_builder(srs_indication_message& msg_) : msg(msg_) {}

  /// \brief Sets the \e SRS.indication basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 Section 3.4.10 in table SRS.indication message body.
  srs_indication_message_builder& set_basic_parameters(uint16_t sfn, uint16_t slot)
  {
    msg.sfn            = sfn;
    msg.slot           = slot;
    msg.control_length = 0;

    return *this;
  }

  /// Adds a SRS PDU to the \e SRS.indication message and returns a SRS PDU builder.
  srs_indication_pdu_builder add_srs_pdu(uint32_t handle, rnti_t rnti)
  {
    auto& pdu = msg.pdus.emplace_back();

    srs_indication_pdu_builder builder(pdu);
    builder.set_basic_parameters(handle, rnti);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
