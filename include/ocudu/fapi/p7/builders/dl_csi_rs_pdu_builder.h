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

#include "ocudu/fapi/p7/builders/tx_precoding_and_beamforming_pdu_builder.h"
#include "ocudu/fapi/p7/messages/dl_csi_rs_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// CSI-RS PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.2.3.
class dl_csi_rs_pdu_builder
{
  dl_csi_rs_pdu& pdu;

public:
  explicit dl_csi_rs_pdu_builder(dl_csi_rs_pdu& pdu_) : pdu(pdu_) {}

  /// Sets the CSI-RS PDU basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.3 in table CSI-RS PDU.
  dl_csi_rs_pdu_builder& set_basic_parameters(uint16_t                         start_rb,
                                              uint16_t                         nof_rbs,
                                              csi_rs_type                      type,
                                              uint8_t                          row,
                                              const bounded_bitset<12, false>& freq_domain,
                                              uint8_t                          symb_l0,
                                              uint8_t                          symb_l1,
                                              csi_rs_cdm_type                  cdm_type,
                                              csi_rs_freq_density_type         freq_density,
                                              uint16_t                         scrambling_id)
  {
    pdu.start_rb     = start_rb;
    pdu.num_rbs      = nof_rbs;
    pdu.type         = type;
    pdu.row          = row;
    pdu.freq_domain  = freq_domain;
    pdu.symb_L0      = symb_l0;
    pdu.symb_L1      = symb_l1;
    pdu.cdm_type     = cdm_type;
    pdu.freq_density = freq_density;
    pdu.scramb_id    = scrambling_id;

    return *this;
  }

  /// Sets the CSI-RS PDU BWP parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.3 in table CSI-RS PDU.
  dl_csi_rs_pdu_builder& set_bwp_parameters(subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.scs = scs;
    pdu.cp  = cp;

    return *this;
  }

  /// Sets the vendor specific CSI-RS PDU BWP parameters and returns a reference to the builder.
  /// \note These parameters are vendor specific.
  dl_csi_rs_pdu_builder& set_vendor_specific_bwp_parameters(unsigned bwp_size, unsigned bwp_start)
  {
    pdu.bwp_size  = bwp_size;
    pdu.bwp_start = bwp_start;

    return *this;
  }

  /// Sets the CSI-RS PDU tx power info parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.3 in table CSI-RS PDU.
  dl_csi_rs_pdu_builder& set_tx_power_info_parameters(int                     power_control_offset,
                                                      power_control_offset_ss power_control_offset_ss)
  {
    pdu.power_control_offset_ss_profile_nr = power_control_offset_ss;
    pdu.power_control_offset_profile_nr    = power_control_offset;

    return *this;
  }

  /// Returns a transmission precoding and beamforming PDU builder of this CSI-RS PDU.
  tx_precoding_and_beamforming_pdu_builder get_tx_precoding_and_beamforming_pdu_builder()
  {
    tx_precoding_and_beamforming_pdu_builder builder(pdu.precoding_and_beamforming);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
