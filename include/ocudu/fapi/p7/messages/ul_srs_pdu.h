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

#include "ocudu/fapi/p7/messages/srs_pdu_report_type.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/srs/srs_configuration.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <bitset>

namespace ocudu {
namespace fapi {

/// SRS parameters in FAPIv4.
struct ul_srs_params_v4 {
  /// Maximum number of UL spatial stream ports.
  static constexpr unsigned MAX_NUM_UL_SPATIAL_STREAM_PORTS = 64;

  struct symbol_info {
    uint16_t srs_bandwidth_start;
    uint8_t  sequence_group;
    uint8_t  sequence_number;
  };

  uint16_t                                             srs_bandwidth_size;
  std::array<symbol_info, 4>                           sym_info;
  uint32_t                                             usage;
  std::bitset<to_value(srs_report_type::last)>         report_type;
  uint8_t                                              singular_value_representation;
  uint8_t                                              iq_representation;
  uint16_t                                             prg_size;
  uint8_t                                              num_total_ue_antennas;
  uint32_t                                             ue_antennas_in_this_srs_resource_set;
  uint32_t                                             sampled_ue_antennas;
  uint8_t                                              report_scope;
  uint8_t                                              num_ul_spatial_streams_ports;
  std::array<uint8_t, MAX_NUM_UL_SPATIAL_STREAM_PORTS> ul_spatial_stream_ports;
};

/// SRS PDU.
struct ul_srs_pdu {
  rnti_t                        rnti;
  uint32_t                      handle;
  uint16_t                      bwp_size;
  uint16_t                      bwp_start;
  subcarrier_spacing            scs;
  cyclic_prefix                 cp;
  uint8_t                       num_ant_ports;
  uint8_t                       num_symbols;
  srs_nof_symbols               num_repetitions;
  uint8_t                       time_start_position;
  uint8_t                       config_index;
  uint16_t                      sequence_id;
  uint8_t                       bandwidth_index;
  tx_comb_size                  comb_size;
  uint8_t                       comb_offset;
  uint8_t                       cyclic_shift;
  uint8_t                       frequency_position;
  uint16_t                      frequency_shift;
  uint8_t                       frequency_hopping;
  srs_group_or_sequence_hopping group_or_sequence_hopping;
  srs_resource_type             resource_type;
  srs_periodicity               t_srs;
  uint16_t                      t_offset;
  // :TODO: beamforming.
  ul_srs_params_v4 srs_params_v4;
};

} // namespace fapi
} // namespace ocudu
