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

#include "ocudu/ran/band_helper.h"
#include "ocudu/ran/carrier_configuration.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/ssb/ssb_properties.h"
#include "ocudu/ran/tdd/tdd_ul_dl_config.h"

namespace ocudu {

/// \brief Main cell parameters from which other cell parameters (e.g. coreset0, BWP RBs) will be derived.
/// \remark Only fields that may affect many different fields in du_cell_config (e.g. number of PRBs) should be added
/// in this struct.
struct cell_config_builder_params {
  /// Physical Cell Identity.
  pci_t pci = 1;
  /// subCarrierSpacingCommon, as per \c MIB, TS 38.331.
  subcarrier_spacing scs_common = subcarrier_spacing::kHz15;
  /// DL carrier configuration.
  /// \remark If nr_band is invalid, an appropriate band is derived from the provided DL ARFCN.
  /// \remark UL carrier is automatically derived.
  carrier_configuration dl_carrier{bs_channel_bandwidth::MHz10, 365000, nr_band::invalid, 1};
  /// offsetToPointA, as per TS 38.211, Section 4.4.4.2; \ref ssb_offset_to_pointA. If not specified, a valid offset
  /// is derived.
  std::optional<ssb_offset_to_pointA> offset_to_point_a;
  /// This is \c controlResourceSetZero, as per TS38.213, Section 13. If not specified, a valid coreset0 is derived.
  std::optional<unsigned> coreset0_index;
  /// Maximum CORESET#0 duration in OFDM symbols to consider when deriving CORESET#0 index.
  uint8_t max_coreset0_duration = 2;
  /// This is \c searchSpaceZero, as per TS38.213, Section 13.
  unsigned search_space0_index = 0;
  /// \brief k_ssb or SSB SubcarrierOffest, as per TS38.211 Section 7.4.3.1. Possible values: {0, ..., 23}. If not
  /// specified, a valid k_ssb is derived.
  std::optional<ssb_subcarrier_offset> k_ssb;
  /// Whether to enable CSI-RS in the cell.
  bool csi_rs_enabled = true;
  /// \brief Minimum k1 value used in the generation of the UE "dl-DataToUl-Ack", as per TS38.213, 9.1.2.1.
  /// Possible values: {1, ..., 15}.
  uint8_t min_k1 = 4;
  /// \brief Minimum k2 value used in the generation of the UE PUSCH time-domain resources. The value of min_k2
  /// should be equal or lower than min_k1. Otherwise, the scheduler is forced to pick higher k1 values, as it cannot
  /// allocate PUCCHs in slots where there is an PUSCH with an already assigned DAI.
  /// Possible values: {1, ..., 32}.
  uint8_t min_k2 = 4;
  /// Defines the TDD DL-UL pattern and periodicity. If no value is set, the cell is in FDD mode.
  std::optional<tdd_ul_dl_config_common> tdd_ul_dl_cfg_common;
  /// Maximum number of DL layers.
  std::optional<unsigned> max_nof_layers;
};

} // namespace ocudu
