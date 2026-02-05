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

#include "ocudu/mac/config/mac_cell_group_params.h"
#include "ocudu/ntn/ntn_cell_params.h"
#include "ocudu/ran/carrier_configuration.h"
#include "ocudu/ran/nr_cgi.h"
#include "ocudu/ran/pci.h"
#include "ocudu/ran/sib/system_info_config.h"
#include "ocudu/ran/ssb/ssb_configuration.h"
#include "ocudu/ran/tac.h"
#include "ocudu/ran/tdd/tdd_ul_dl_config.h"
#include "ocudu/scheduler/config/bwp_builder_params.h"
#include "ocudu/scheduler/config/bwp_configuration.h"
#include "ocudu/scheduler/config/pucch_builder_params.h"
#include "ocudu/scheduler/config/serving_cell_config.h"
#include "ocudu/scheduler/config/slice_rrm_policy_config.h"
#include "ocudu/scheduler/config/srs_builder_params.h"

namespace ocudu {
namespace odu {

/// Parameters that are used to initialize or build the \c PhysicalCellGroupConfig, TS 38.331.
struct phy_cell_group_params {
  /// \brief \c p-NR-FR1, part \c PhysicalCellGroupConfig, TS 38.331.
  /// The maximum total TX power to be used by the UE in this NR cell group across all serving cells in FR1.
  std::optional<bounded_integer<int, -30, 33>> p_nr_fr1;
};

/// Cell Configuration, including common and UE-dedicated configs, that the DU will use to generate other configs for
/// other layers (e.g. scheduler).
struct du_cell_config {
  pci_t               pci;
  tac_t               tac;
  nr_cell_global_id_t nr_cgi;

  carrier_configuration dl_carrier;
  carrier_configuration ul_carrier;

  /// subcarrierSpacing for common, used for initial access and broadcast message.
  subcarrier_spacing scs_common;
  ssb_configuration  ssb_cfg;

  /// Whether the DU automatically attempts to activate the cell or waits for a command from the SMO.
  /// Note: If set to false, the DU won't add this cell to the list of served cells in the F1 Setup Request.
  bool enabled = true;

  dmrs_typeA_position dmrs_typeA_pos;

  /// CORESET#0 index of Table 13-{1, ..., 10}, TS 38.213.
  unsigned coreset0_idx;

  /// SearcSpace#0 index of Table 13-{11, ..., 15}, TS 38.213.
  unsigned searchspace0_idx;

  /// Parameters used to pack MIB.
  /// "cellBarred" as per MIB, TS 38.331. true = barred; false = notBarred.
  bool cell_barred;
  /// "intraFreqReselection" as per MIB, TS 38.331. true = allowed; false = notAllowed.
  bool intra_freq_resel;

  /// \c cellSelectionInfo, \c SIB1, as per TS 38.331.
  cell_selection_info cell_sel_info;

  /// \c cellAccessRelatedInfo, sent in \c SIB1, as per TS 38.331.
  cell_access_related_info cell_acc_rel_info;

  /// Content and scheduling information of SI-messages.
  std::optional<si_scheduling_info_config> si_config;

  /// \c ueTimersAndConstants, sent in \c SIB1, as per TS 38.331.
  ue_timers_and_constants_config ue_timers_and_constants;

  /// Cell-specific DL and UL configuration used by common searchSpaces.
  dl_config_common dl_cfg_common;
  ul_config_common ul_cfg_common;

  /// Defines the TDD DL-UL pattern and periodicity. If no value is set, the cell is in FDD mode.
  std::optional<tdd_ul_dl_config_common> tdd_ul_dl_cfg_common;

  /// UE-dedicated serving cell configuration.
  serving_cell_config ue_ded_serv_cell_cfg;

  /// Parameters to initialize/build the \c phy_cell_group.
  phy_cell_group_params pcg_params;

  /// Parameters to initialize/build the \c mac_cell_group_config.
  mac_cell_group_params mcg_params;

  /// Parameters for PUCCH-Config generation.
  pucch_builder_params pucch_cfg;

  /// Parameters for SRS-Config generation.
  srs_builder_params srs_cfg;

  /// Parameters for the initial BWP config generation.
  bwp_builder_params init_bwp_builder;

  /// NTN configuration for this cell. When empty, the cell operates in terrestrial mode.
  std::optional<ntn_cell_params> ntn_params;

  /// PUSCH Maximum of transmission layers. Limits the PUSCH maximum rank the UE is configured with.
  unsigned pusch_max_nof_layers = 1;

  /// Whether contention-free random access is enabled for this cell.
  bool cfra_enabled = true;

  /// Whether eDRX paging is enabled.
  bool edrx_enabled = false;

  /// List of RAN slices to support in the scheduler.
  std::vector<slice_rrm_policy_config> rrm_policy_members;
};

} // namespace odu
} // namespace ocudu
