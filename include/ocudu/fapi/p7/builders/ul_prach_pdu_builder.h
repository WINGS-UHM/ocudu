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

#include "ocudu/fapi/p7/messages/ul_prach_pdu.h"
#include <limits>
#include <optional>

namespace ocudu {
namespace fapi {

// :TODO: Review the builders documentation so it matches the UCI builder.

/// PRACH PDU builder that helps to fill in the parameters specified in SCF-222 v4.0 section 3.4.3.1.
class ul_prach_pdu_builder
{
  ul_prach_pdu& pdu;

public:
  explicit ul_prach_pdu_builder(ul_prach_pdu& pdu_) : pdu(pdu_) {}

  /// Sets the PRACH PDU basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.1 in table PRACH PDU.
  ul_prach_pdu_builder& set_basic_parameters(pci_t             pci,
                                             uint8_t           num_occasions,
                                             prach_format_type format_type,
                                             uint8_t           index_fd_ra,
                                             uint8_t           prach_start_symbol,
                                             uint16_t          num_cs)
  {
    pdu.phys_cell_id                = pci;
    pdu.num_prach_ocas              = num_occasions;
    pdu.prach_format                = format_type;
    pdu.index_fd_ra                 = index_fd_ra;
    pdu.prach_start_symbol          = prach_start_symbol;
    pdu.num_cs                      = num_cs;
    pdu.is_msg_a_prach              = 0;
    pdu.has_msg_a_pusch_beamforming = false;

    return *this;
  }

  /// Sets the PRACH PDU maintenance v3 basic parameters and returns a reference to the builder.
  /// \note These parameters are specified in SCF-222 v4.0 section 3.4.3.1 in table PRACH maintenance FAPIv3.
  ul_prach_pdu_builder& set_maintenance_v3_basic_parameters(uint32_t                handle,
                                                            prach_config_scope_type prach_config_scope,
                                                            uint16_t                prach_res_config_index,
                                                            uint8_t                 num_fd_ra,
                                                            std::optional<uint8_t>  start_preamble_index,
                                                            uint8_t                 num_preambles_indices)
  {
    auto& v3                  = pdu.maintenance_v3;
    v3.handle                 = handle;
    v3.prach_config_scope     = prach_config_scope;
    v3.prach_res_config_index = prach_res_config_index;
    v3.num_fd_ra              = num_fd_ra;
    v3.start_preamble_index =
        (start_preamble_index) ? start_preamble_index.value() : std::numeric_limits<uint8_t>::max();
    v3.num_preamble_indices = num_preambles_indices;

    return *this;
  }

  //: TODO: beamforming
  //: TODO: uplink spatial assignment
};

} // namespace fapi
} // namespace ocudu
