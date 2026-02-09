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

#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/pdcch/aggregation_level.h"
#include "ocudu/ran/pdcch/dci_packing.h"
#include "ocudu/ran/pdcch/pdcch_context.h"
#include "ocudu/ran/resource_allocation/ofdm_symbol_range.h"
#include "ocudu/ran/rnti.h"
#include "ocudu/ran/slot_pdu_capacity_constants.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace fapi {

/// Downlink DCI PDU configuration.
struct dl_dci_pdu {
  /// PDCCH PDU profile SSS power parameters.
  struct power_profile_sss {
    float dmrs_power_offset_db;
    float data_power_offset_db;
  };

  /// PDCCH PDU profile NR power parameters.
  struct power_profile_nr {
    int8_t power_control_offset_ss;
  };

  rnti_t                                            rnti;
  uint16_t                                          nid_pdcch_data;
  uint16_t                                          nid_pdcch_dmrs;
  uint16_t                                          nrnti_pdcch_data;
  uint8_t                                           cce_index;
  aggregation_level                                 dci_aggregation_level;
  tx_precoding_and_beamforming_pdu                  precoding_and_beamforming;
  std::variant<power_profile_nr, power_profile_sss> power_config;
  dci_payload                                       payload;
  /// Vendor specific parameters.
  std::optional<pdcch_context> context;
};

/// Downlink PDCCH PDU information.
struct dl_pdcch_pdu {
  /// Holds the parameters that are needed for PDUs transmitted in CORESET 0.
  struct mapping_coreset_0 {
    coreset_configuration::interleaved_mapping_type interleaved;
  };

  /// Holds the parameters that are needed for PDUs with interleaving.
  struct mapping_interleaved {
    coreset_configuration::interleaved_mapping_type interleaved;
  };

  /// Holds the parameters that are needed for PDUs with no interleaving.
  struct mapping_non_interleaved {
    uint8_t reg_bundle_sz;
  };

  crb_interval                                                                  coreset_bwp;
  subcarrier_spacing                                                            scs;
  cyclic_prefix                                                                 cp;
  ofdm_symbol_range                                                             symbols;
  freq_resource_bitmap                                                          freq_domain_resource;
  std::variant<mapping_coreset_0, mapping_interleaved, mapping_non_interleaved> mapping;
  coreset_configuration::precoder_granularity_type                              precoder_granularity;
  dl_dci_pdu                                                                    dl_dci;
};

} // namespace fapi
} // namespace ocudu
