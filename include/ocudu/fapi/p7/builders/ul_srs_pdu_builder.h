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

#include "ocudu/fapi/p7/messages/ul_srs_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// Uplink SRS PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.3.3.
class ul_srs_pdu_builder
{
  ul_srs_pdu& pdu;

public:
  explicit ul_srs_pdu_builder(ul_srs_pdu& pdu_) : pdu(pdu_) { pdu.srs_params_v4.report_type = 0; }

  /// Sets the SRS PDU basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder& set_basic_parameters(rnti_t rnti, uint32_t handle)
  {
    pdu.rnti   = rnti;
    pdu.handle = handle;

    return *this;
  }

  /// Sets the SRS PDU BWP parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder&
  set_bwp_parameters(uint16_t bwp_size, uint16_t bwp_start, subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.bwp_size  = bwp_size;
    pdu.bwp_start = bwp_start;
    pdu.scs       = scs;
    pdu.cp        = cp;

    return *this;
  }

  /// Sets the SRS PDU timing parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder& set_timing_params(unsigned time_start_position, srs_periodicity t_srs, unsigned t_offset)
  {
    pdu.time_start_position = time_start_position;
    pdu.t_srs               = t_srs;
    pdu.t_offset            = t_offset;

    return *this;
  }

  /// Sets the SRS PDU comb parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder& set_comb_params(tx_comb_size comb_size, unsigned comb_offset)
  {
    pdu.comb_size   = comb_size;
    pdu.comb_offset = comb_offset;

    return *this;
  }

  /// Sets the SRS report types and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v8.0 section 3.4.3.4 in table SRS PDU.
  ul_srs_pdu_builder& set_report_params(bool enable_normalized_iq_matrix_report, bool enable_positioning_report)
  {
    if (enable_normalized_iq_matrix_report) {
      pdu.srs_params_v4.report_type.set(to_value(srs_report_type::normalized_channel_iq_matrix));
    }

    if (enable_positioning_report) {
      pdu.srs_params_v4.report_type.set(to_value(srs_report_type::positioning));
    }

    return *this;
  }

  /// Sets the SRS PDU frequency parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder& set_frequency_params(unsigned                      frequency_position,
                                           unsigned                      frequency_shift,
                                           unsigned                      frequency_hopping,
                                           srs_group_or_sequence_hopping group_or_sequence_hopping)
  {
    pdu.frequency_position        = frequency_position;
    pdu.frequency_shift           = frequency_shift;
    pdu.frequency_hopping         = frequency_hopping;
    pdu.group_or_sequence_hopping = group_or_sequence_hopping;

    return *this;
  }

  /// Sets the SRS PDU parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.3 in table SRS PDU.
  ul_srs_pdu_builder& set_srs_params(unsigned          nof_antenna_ports,
                                     unsigned          nof_symbols,
                                     srs_nof_symbols   nof_repetitions,
                                     unsigned          config_index,
                                     unsigned          sequence_id,
                                     unsigned          bandwidth_index,
                                     unsigned          cyclic_shift,
                                     srs_resource_type resource_type)
  {
    pdu.num_ant_ports   = nof_antenna_ports;
    pdu.num_symbols     = nof_symbols;
    pdu.num_repetitions = nof_repetitions;
    pdu.config_index    = config_index;
    pdu.sequence_id     = sequence_id;
    pdu.bandwidth_index = bandwidth_index;
    pdu.cyclic_shift    = cyclic_shift;
    pdu.resource_type   = resource_type;

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
