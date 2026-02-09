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

#include "ocudu/fapi/p7/messages/power_control_offset_ss.h"
#include "ocudu/fapi/p7/messages/pxsch_parameters.h"
#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/pdsch/pdsch_context.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/sch/ldpc_base_graph.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <bitset>
#include <variant>

namespace ocudu {
namespace fapi {

enum class pdsch_trans_type : uint8_t {
  non_interleaved_common_ss,
  non_interleaved_other,
  interleaved_common_type0_coreset0,
  interleaved_common_any_coreset0_present,
  interleaved_common_any_coreset0_not_present,
  interleaved_other
};

/// \note For this release num_coreset_rm_patterns = 0.
/// \note For this release num_prb_sym_rm_patts_by_value = 0.
struct dl_pdsch_maintenance_parameters_v3 {
  static constexpr unsigned MAX_SIZE_SSB_PDU_FOR_RM = 8U;
  /// Bit position of the first TB inside the tb_crc_required bitmap.
  static constexpr unsigned TB_BITMAP_FIRST_TB_BIT = 0U;
  /// Bit position of the second TB inside the tb_crc_required bitmap.
  static constexpr unsigned TB_BITMAP_SECOND_TB_BIT = 1U;

  pdsch_trans_type                              trans_type;
  uint16_t                                      coreset_start_point;
  uint16_t                                      initial_dl_bwp_size;
  ldpc_base_graph_type                          ldpc_base_graph;
  units::bytes                                  tb_size_lbrm_bytes;
  uint8_t                                       tb_crc_required;
  std::array<uint16_t, MAX_SIZE_SSB_PDU_FOR_RM> ssb_pdus_for_rate_matching;
  uint16_t                                      ssb_config_for_rate_matching;
  uint8_t                                       prb_sym_rm_pattern_bitmap_size_byref;
  //: TODO: determine max size of this array
  static_vector<uint8_t, 16> prb_sym_rm_patt_bmp_byref;
  uint8_t                    num_prb_sym_rm_patts_by_value;
  uint8_t                    num_coreset_rm_patterns;
  uint16_t                   pdcch_pdu_index;
  uint16_t                   dci_index;
  uint8_t                    lte_crs_rm_pattern_bitmap_size;
  //: TODO: determine max size of this array
  static_vector<uint8_t, 16>  lte_crs_rm_pattern;
  static_vector<uint16_t, 16> csi_for_rm;
  uint8_t                     max_num_cbg_per_tb;
  //: TODO: determine max size of this array.
  static_vector<uint8_t, 16> cbg_tx_information;
};

struct dl_pdsch_parameters_v4 {
  uint8_t coreset_rm_pattern_bitmap_size_by_ref;
  //: TODO: determine max size of this array
  static_vector<uint8_t, 16> coreset_rm_pattern_bmp_by_ref;
  uint8_t                    lte_crs_mbsfn_derivation_method;
  // :TODO: determine max size of this array. This size is either 0 or lte_crs_rm_pattern.size() parameter in
  // maintenance v3.
  static_vector<uint8_t, 16> lte_crs_mbsfn_pattern;
  //: TODO: MU-MIMO fields
};

/// Codeword information.
struct dl_pdsch_codeword {
  uint16_t     target_code_rate;
  uint8_t      qam_mod_order;
  uint8_t      mcs_index;
  uint8_t      mcs_table;
  uint8_t      rv_index;
  units::bytes tb_size;
};

enum class inline_tb_crc_type : uint8_t { data_payload, control_message };
enum class pdsch_ref_point_type : uint8_t { point_a, subcarrier_0 };

/// Downlink PDSCH PDU information.
struct dl_pdsch_pdu {
  static constexpr unsigned BITMAP_SIZE = 2U;

  /// Bit position of PTRS in the PDU bitmap.
  static constexpr unsigned PDU_BITMAP_PTRS_BIT = 0U;
  /// Bit position of CBG retransmission control in the PDU bitmap.
  static constexpr unsigned PDU_BITMAP_CBG_RETX_CTRL_BIT = 1U;
  /// Bit position of the first TB in the is_last_cb_present bitmap.
  static constexpr unsigned LAST_CB_BITMAP_FIRST_TB_BIT = 0U;
  /// Bit position of the second TB in the is_last_cb_present bitmap.
  static constexpr unsigned LAST_CB_BITMAP_SECOND_TB_BIT = 1U;

  /// Maximum number of codewords per PDU.
  static constexpr unsigned MAX_NUM_CW_PER_PDU = 2;
  /// Maximum size of the RB bitmap in Bytes.
  static constexpr unsigned MAX_SIZE_RB_BITMAP = 36;
  /// Maximum size of DL TB CRC.
  static constexpr unsigned MAX_SIZE_DL_TB_CRC = 2;

  /// Profile NR power parameters.
  struct power_profile_nr {
    int                     power_control_offset_profile_nr;
    power_control_offset_ss power_control_offset_ss_profile_nr;
  };

  /// Profile SSS power parameters.
  struct power_profile_sss {
    float dmrs_power_offset_sss_db;
    float data_power_offset_sss_db;
  };

  std::bitset<BITMAP_SIZE>                             pdu_bitmap;
  rnti_t                                               rnti;
  uint16_t                                             pdu_index;
  crb_interval                                         bwp;
  subcarrier_spacing                                   scs;
  cyclic_prefix                                        cp;
  static_vector<dl_pdsch_codeword, MAX_NUM_CW_PER_PDU> cws;
  uint16_t                                             nid_pdsch;
  uint8_t                                              num_layers;
  uint8_t                                              transmission_scheme;
  pdsch_ref_point_type                                 ref_point;
  uint16_t                                             dl_dmrs_symb_pos;
  uint16_t                                             pdsch_dmrs_scrambling_id;
  dmrs_cfg_type                                        dmrs_type;
  uint16_t                                             pdsch_dmrs_scrambling_id_compl;
  low_papr_dmrs_type                                   low_papr_dmrs;
  uint8_t                                              nscid;
  uint8_t                                              num_dmrs_cdm_grps_no_data;
  uint16_t                                             dmrs_ports;
  resource_allocation_type                             resource_alloc;
  std::array<uint8_t, MAX_SIZE_RB_BITMAP>              rb_bitmap;
  vrb_interval                                         vrbs;
  vrb_to_prb_mapping_type                              vrb_to_prb_mapping;
  ofdm_symbol_range                                    symbols;
  std::variant<power_profile_nr, power_profile_sss>    power_config;
  // :TODO: PTRS
  tx_precoding_and_beamforming_pdu         precoding_and_beamforming;
  uint8_t                                  is_last_cb_present;
  inline_tb_crc_type                       is_inline_tb_crc;
  std::array<uint32_t, MAX_SIZE_DL_TB_CRC> dl_tb_crc_cw;
  dl_pdsch_maintenance_parameters_v3       pdsch_maintenance_v3;
  // :TODO: Rel16 PDSCH params v3
  dl_pdsch_parameters_v4 pdsch_parameters_v4;
  /// Vendor specific parameters.
  std::optional<pdsch_context> context;
};

} // namespace fapi
} // namespace ocudu
