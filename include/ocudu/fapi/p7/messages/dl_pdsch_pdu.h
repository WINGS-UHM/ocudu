// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

#pragma once

#include "ocudu/fapi/p7/messages/dmrs_definitions.h"
#include "ocudu/fapi/p7/messages/power_control_offset_ss.h"
#include "ocudu/fapi/p7/messages/resource_allocation_types.h"
#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/dmrs/dmrs.h"
#include "ocudu/ran/pdsch/pdsch_constants.h"
#include "ocudu/ran/pdsch/pdsch_context.h"
#include "ocudu/ran/pdsch/pdsch_mcs.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/resource_allocation/vrb_to_prb.h"
#include "ocudu/ran/sch/ldpc_base_graph.h"
#include "ocudu/ran/sch/modulation_scheme.h"
#include "ocudu/ran/sch/sch_mcs.h"
#include "ocudu/ran/subcarrier_spacing.h"
#include <variant>

namespace ocudu {
namespace fapi {

/// Codeword information.
struct dl_pdsch_codeword {
  modulation_scheme qam_mod_order;
  sch_mcs_index     mcs_index;
  pdsch_mcs_table   mcs_table;
  uint8_t           rv_index;
  units::bytes      tb_size;
};

enum class pdsch_ref_point_type : uint8_t { point_a, subcarrier_0 };

/// Downlink PDSCH PDU information.
struct dl_pdsch_pdu {
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

  /// VRB to PRB mapping non interleaved common search space.
  struct non_interleaved_common_ss {
    unsigned N_start_coreset;
  };

  /// VRB to PRB mapping non interleaved other.
  struct non_interleaved_other {};

  /// VRB to PRB mapping interleaved common type 0 CORESET 0.
  struct interleaved_common_type0_coreset0 {
    unsigned N_start_coreset;
    unsigned N_bwp_init_size;
  };

  /// VRB to PRB mapping interleaved common any CORESET 0 present.
  struct interleaved_common_any_coreset0_present {
    unsigned N_start_coreset;
    unsigned N_bwp_init_size;
  };

  /// VRB to PRB mapping interleaved other.
  struct interleaved_other {};

  using vrb_to_prb_mapping_t = std::variant<non_interleaved_common_ss,
                                            non_interleaved_other,
                                            interleaved_common_type0_coreset0,
                                            interleaved_common_any_coreset0_present,
                                            interleaved_other>;

  rnti_t                                                               rnti;
  crb_interval                                                         bwp;
  subcarrier_spacing                                                   scs;
  cyclic_prefix                                                        cp;
  static_vector<dl_pdsch_codeword, pdsch_constants::MAX_NOF_CODEWORDS> cws;
  uint16_t                                                             nid_pdsch;
  uint8_t                                                              num_layers;
  pdsch_ref_point_type                                                 ref_point;
  dmrs_symbol_mask                                                     dl_dmrs_symb_pos;
  uint16_t                                                             pdsch_dmrs_scrambling_id;
  dmrs_config_type                                                     dmrs_type;
  uint8_t                                                              nscid;
  uint8_t                                                              num_dmrs_cdm_grps_no_data;
  dmrs_ports_mask                                                      dmrs_ports;
  resource_allocation_type_1                                           resource_alloc;
  vrb_to_prb::mapping_type                                             vrb_to_prb_mapping;
  ofdm_symbol_range                                                    symbols;
  std::variant<power_profile_nr, power_profile_sss>                    power_config;
  tx_precoding_and_beamforming_pdu                                     precoding_and_beamforming;
  units::bytes                                                         tb_size_lbrm;
  ldpc_base_graph_type                                                 ldpc_base_graph;
  uint16_t                                                             nof_csi_pdus_for_rm;
  vrb_to_prb_mapping_t                                                 mapping;
  /// Vendor specific parameters.
  std::optional<pdsch_context> context;
};

} // namespace fapi
} // namespace ocudu
