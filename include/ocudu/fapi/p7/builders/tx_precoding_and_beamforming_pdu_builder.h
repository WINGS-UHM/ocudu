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
#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// Helper class to fill the transmission precoding and beamforming parameters specified in SCF-222 v4.0
/// section 3.4.2.5.
class tx_precoding_and_beamforming_pdu_builder
{
  tx_precoding_and_beamforming_pdu& pdu;

public:
  explicit tx_precoding_and_beamforming_pdu_builder(tx_precoding_and_beamforming_pdu& pdu_) : pdu(pdu_)
  {
    // Mark the tx precoding and beamforming pdu as used when this builder is called.
    pdu.trp_scheme = 0U;
    // Initialize number of digital beamforming interfaces.
    pdu.dig_bf_interfaces = 0U;
  }

  /// Sets the basic parameters for the fields of the tranmission precoding and beamforming PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.5, in table Tx precoding and beamforming PDU.
  tx_precoding_and_beamforming_pdu_builder& set_basic_parameters(unsigned prg_size, unsigned dig_bf_interfaces)
  {
    pdu.prg_size          = prg_size;
    pdu.dig_bf_interfaces = dig_bf_interfaces;

    return *this;
  }

  /// Adds a PRG to the transmission precoding and beamforming PDU.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.2.5, in table Tx precoding and beamforming PDU.
  tx_precoding_and_beamforming_pdu_builder& add_prg(unsigned pm_index, span<const uint16_t> beam_index)
  {
    tx_precoding_and_beamforming_pdu::prgs_info& prg = pdu.prgs.emplace_back();

    ocudu_assert(pdu.dig_bf_interfaces == beam_index.size(),
                 "Error number of beam indexes={} does not match the expected={}",
                 beam_index.size(),
                 pdu.dig_bf_interfaces);

    prg.pm_index = pm_index;
    prg.beam_index.assign(beam_index.begin(), beam_index.end());

    return *this;
  }
};

} // namespace fapi
} // namespace ocudu
