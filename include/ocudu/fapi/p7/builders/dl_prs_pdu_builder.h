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
#include "ocudu/fapi/p7/messages/dl_prs_pdu.h"

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// PRS PDU builder that helps to fill in the parameters specified in SCF-222 v8.0 section 3.4.2.4a.
class dl_prs_pdu_builder
{
  dl_prs_pdu& pdu;

public:
  explicit dl_prs_pdu_builder(dl_prs_pdu& pdu_) : pdu(pdu_) {}

  /// Sets the PRS PDU basic parameters and returns a reference to the builder.
  dl_prs_pdu_builder& set_basic_parameters(subcarrier_spacing scs, cyclic_prefix cp)
  {
    pdu.scs = scs;
    pdu.cp  = cp;

    return *this;
  }

  /// Sets the PRS PDU N_ID parameter and returns a reference to the builder.
  dl_prs_pdu_builder& set_n_id(unsigned n_id)
  {
    pdu.nid_prs = n_id;

    return *this;
  }

  /// Sets the PRS PDU symbol parameters and returns a reference to the builder.
  dl_prs_pdu_builder& set_symbol_parameters(prs_num_symbols nof_symbols, unsigned first_symbol)
  {
    pdu.num_symbols  = nof_symbols;
    pdu.first_symbol = first_symbol;

    return *this;
  }

  /// Sets the PRS PDU RB parameters and returns a reference to the builder.
  dl_prs_pdu_builder& set_rb_parameters(unsigned nof_rb, unsigned start_rb)
  {
    pdu.num_rbs  = nof_rb;
    pdu.start_rb = start_rb;

    return *this;
  }

  /// Sets the PRS PDU power offset parameter and returns a reference to the builder.
  dl_prs_pdu_builder& set_power_offset(std::optional<float> power_offset)
  {
    pdu.prs_power_offset = power_offset;

    return *this;
  }

  /// Sets the PRS PDU transmission comb parameters and returns a reference to the builder.
  dl_prs_pdu_builder& set_comb_parameters(prs_comb_size comb_size, unsigned comb_offset)
  {
    pdu.comb_size   = comb_size;
    pdu.comb_offset = comb_offset;

    return *this;
  }

  /// Returns a transmission precoding and beamforming PDU builder of this PRS PDU.
  tx_precoding_and_beamforming_pdu_builder get_tx_precoding_and_beamforming_pdu_builder()
  {
    tx_precoding_and_beamforming_pdu_builder builder(pdu.precoding_and_beamforming);

    return builder;
  }
};

} // namespace fapi
} // namespace ocudu
