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

#include "ocudu/fapi/p7/messages/srs_indication.h"

namespace ocudu {
namespace fapi {

/// SRS indication PDU builder that helps fill in the parameters specified in SCF-222 v4.0 section 3.4.10.
class srs_indication_pdu_builder
{
  srs_indication_pdu& pdu;

public:
  explicit srs_indication_pdu_builder(srs_indication_pdu& pdu_) : pdu(pdu_) {}

  /// \brief Sets the SRS indication PDU UE specific parameters and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.10.
  srs_indication_pdu_builder& set_ue_specific_parameters(rnti_t rnti)
  {
    pdu.rnti = rnti;

    return *this;
  }

  /// \brief Sets the SRS indication PDU metrics parameters and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.10.
  srs_indication_pdu_builder& set_metrics_parameters(std::optional<phy_time_unit> timing_advance_offset)
  {
    pdu.timing_advance_offset = timing_advance_offset;

    return *this;
  }

  /// \brief Sets the SRS indication PDU normalized channel I/Q matrix and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.10 Table 3-132.
  srs_indication_pdu_builder& set_codebook_report_matrix(const srs_channel_matrix& matrix)
  {
    pdu.matrix = matrix;

    return *this;
  }

  /// \brief Sets the SRS indication PDU positioning report and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v8.0 Section 3.4.10 Table 3-209.
  srs_indication_pdu_builder& set_positioning_report_parameters(std::optional<phy_time_unit> ul_relative_toa,
                                                                std::optional<float>         rsrp)
  {
    pdu.positioning = std::make_optional<srs_positioning_report>({.ul_relative_toa = ul_relative_toa, .rsrp = rsrp});

    return *this;
  }
};

/// SRS.indication message builder that helps to fill in the parameters specified in SCF-222 v4.0 Section 3.4.10.
class srs_indication_builder
{
  srs_indication& msg;

public:
  explicit srs_indication_builder(srs_indication& msg_) : msg(msg_) {}

  /// \brief Sets the \e SRS.indication basic parameters and returns a reference to the builder.
  ///
  /// These parameters are specified in SCF-222 v4.0 Section 3.4.10 in table SRS.indication message body.
  srs_indication_builder& set_basic_parameters(slot_point slot)
  {
    msg.slot = slot;

    return *this;
  }

  /// Adds a SRS PDU to the \e SRS.indication message and returns a SRS PDU builder.
  srs_indication_pdu_builder add_srs_pdu(rnti_t rnti)
  {
    auto& pdu = msg.pdus.emplace_back();

    srs_indication_pdu_builder builder(pdu);
    builder.set_ue_specific_parameters(rnti);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
