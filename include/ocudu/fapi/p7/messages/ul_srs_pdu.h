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

#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/srs/srs_configuration.h"
#include "ocudu/ran/srs/srs_resource_configuration.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <bitset>

namespace ocudu {
namespace fapi {

/// SRS PDU.
struct ul_srs_pdu {
  rnti_t                                        rnti;
  uint32_t                                      handle = 0U;
  crb_interval                                  bwp;
  subcarrier_spacing                            scs;
  cyclic_prefix                                 cp;
  srs_resource_configuration::one_two_four_enum num_ant_ports;
  ofdm_symbol_range                             ofdm_symbols;
  srs_nof_symbols                               num_repetitions;
  uint8_t                                       time_start_position;
  uint8_t                                       config_index;
  uint16_t                                      sequence_id;
  uint8_t                                       bandwidth_index;
  tx_comb_size                                  comb_size;
  uint8_t                                       comb_offset;
  uint8_t                                       cyclic_shift;
  uint8_t                                       frequency_position;
  uint16_t                                      frequency_shift;
  uint8_t                                       frequency_hopping;
  srs_group_or_sequence_hopping                 group_or_sequence_hopping;
  srs_resource_type                             resource_type;
  srs_periodicity                               t_srs;
  uint16_t                                      t_offset;
  bool                                          enable_normalized_iq_matrix_report{false};
  bool                                          enable_positioning_report{false};
};

} // namespace fapi
} // namespace ocudu
