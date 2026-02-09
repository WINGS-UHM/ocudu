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

#include "ocudu/fapi/p7/messages/pxsch_parameters.h"
#include "ocudu/fapi/p7/messages/uci_part1_to_part2_correspondence_v3.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/pusch/pusch_context.h"
#include "ocudu/ran/pusch/pusch_mcs.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/sch/ldpc_base_graph.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include "ocudu/ran/uci/uci_configuration.h"
#include "ocudu/support/units.h"
#include <bitset>

namespace ocudu {
namespace fapi {

/// Uplink PUSCH data information.
struct ul_pusch_data {
  /// Maximum number of CB.
  //: TODO: determine size of this array
  static constexpr unsigned MAX_NUM_CB = 128;

  uint8_t                            rv_index;
  uint8_t                            harq_process_id;
  bool                               new_data;
  units::bytes                       tb_size;
  uint16_t                           num_cb;
  static_vector<uint8_t, MAX_NUM_CB> cb_present_and_position;
};

/// Uplink PUSCH UCI information.
struct ul_pusch_uci {
  uint16_t          harq_ack_bit_length;
  uint16_t          csi_part1_bit_length;
  uint16_t          flags_csi_part2;
  alpha_scaling_opt alpha_scaling;
  uint8_t           beta_offset_harq_ack;
  uint8_t           beta_offset_csi1;
  uint8_t           beta_offset_csi2;
};

enum class ul_ptrs_power_type : uint8_t { dB0, dB3, dB4_77, dB6 };

/// Uplink PUSCH PTRS information.
struct ul_pusch_ptrs {
  /// Per-port specific information.
  struct ptrs_port_info {
    uint16_t ptrs_port_index;
    uint8_t  ptrs_dmrs_port;
    uint8_t  ptrs_re_offset;
  };

  static_vector<ptrs_port_info, 2> port_info;
  uint8_t                          ptrs_time_density;
  uint8_t                          ptrs_freq_density;
  ul_ptrs_power_type               ul_ptrs_power;
};

/// Uplink PUSCH DFTs-OFDM information.
struct ul_pusch_dfts_ofdm {
  uint8_t  low_papr_group_number;
  uint16_t low_papr_sequence_number;
  uint8_t  ul_ptrs_sample_density;
  uint8_t  ul_ptrs_time_density_transform_precoding;
};

/// PUSCH PDU maintenance information added in FAPIv3.
struct ul_pusch_maintenance_v3 {
  uint8_t              pusch_trans_type;
  uint16_t             delta_bwp0_start_from_active_bwp;
  uint16_t             initial_ul_bwp_size;
  uint8_t              group_or_sequence_hopping;
  uint16_t             pusch_second_hop_prb;
  ldpc_base_graph_type ldpc_base_graph;
  units::bytes         tb_size_lbrm_bytes;
};

/// PUSCH PDU parameters added in FAPIv4.
struct ul_pusch_params_v4 {
  /// Maximum number of spatial streams.
  static constexpr unsigned MAX_NUM_SPATIAL_STREAMS = 64;

  bool                                         cb_crc_status_request;
  uint32_t                                     srs_tx_ports;
  uint8_t                                      ul_tpmi_index;
  uint8_t                                      num_ul_spatial_streams_ports;
  std::array<uint8_t, MAX_NUM_SPATIAL_STREAMS> ul_spatial_stream_ports;
};

/// Uplink PUSCH PDU information.
struct ul_pusch_pdu {
  static constexpr unsigned RB_BITMAP_SIZE_IN_BYTES = 36U;
  static constexpr unsigned BITMAP_SIZE             = 4U;
  /// Bit position of the pdu_bitmap property.
  static constexpr unsigned PUSCH_DATA_BIT = 0U;
  static constexpr unsigned PUSCH_UCI_BIT  = 1U;
  static constexpr unsigned PUSCH_PTRS_BIT = 2U;
  static constexpr unsigned DFTS_OFDM_BIT  = 3U;

  std::bitset<BITMAP_SIZE>                     pdu_bitmap;
  rnti_t                                       rnti;
  uint32_t                                     handle = 0;
  crb_interval                                 bwp;
  subcarrier_spacing                           scs;
  cyclic_prefix                                cp;
  uint16_t                                     target_code_rate;
  modulation_scheme                            qam_mod_order;
  uint8_t                                      mcs_index;
  pusch_mcs_table                              mcs_table;
  bool                                         transform_precoding;
  uint16_t                                     nid_pusch;
  uint8_t                                      num_layers;
  uint16_t                                     ul_dmrs_symb_pos;
  dmrs_cfg_type                                dmrs_type;
  uint16_t                                     pusch_dmrs_scrambling_id;
  uint16_t                                     pusch_dmrs_scrambling_id_complement;
  low_papr_dmrs_type                           low_papr_dmrs;
  uint16_t                                     pusch_dmrs_identity;
  uint8_t                                      nscid;
  uint8_t                                      num_dmrs_cdm_grps_no_data;
  uint16_t                                     dmrs_ports;
  resource_allocation_type                     resource_alloc;
  std::array<uint8_t, RB_BITMAP_SIZE_IN_BYTES> rb_bitmap;
  vrb_interval                                 vrbs;
  vrb_to_prb_mapping_type                      vrb_to_prb_mapping;
  bool                                         intra_slot_frequency_hopping;
  uint16_t                                     tx_direct_current_location;
  bool                                         uplink_frequency_shift_7p5kHz;
  ofdm_symbol_range                            symbols;
  ul_pusch_data                                pusch_data;
  ul_pusch_uci                                 pusch_uci;
  ul_pusch_ptrs                                pusch_ptrs;
  ul_pusch_dfts_ofdm                           pusch_ofdm;
  //: TODO: beamforming struct
  ul_pusch_maintenance_v3              pusch_maintenance_v3;
  ul_pusch_params_v4                   pusch_params_v4;
  uci_part1_to_part2_correspondence_v3 uci_correspondence;
  // Vendor specific parameters.
  std::optional<pusch_context> context;
};

} // namespace fapi
} // namespace ocudu
