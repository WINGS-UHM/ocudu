// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/pusch/tx_scheme_configuration.h"
#include "ocudu/ran/srs/srs_configuration.h"
#include <optional>

namespace ocudu {

// Strong types for UCI specific PUCCH resource IDs.
struct pucch_resource_set_config_id_tag;
using pucch_resource_set_config_id = strong_type<uint8_t, struct pucch_res_set_cfg_id_tag, strong_equality>;
struct pucch_sr_resource_id_tag;
using pucch_sr_resource_id = strong_type<uint8_t, struct pucch_sr_resource_id_tag, strong_equality>;
struct pucch_csi_resource_id_tag;
using pucch_csi_resource_id = strong_type<uint8_t, struct pucch_csi_resource_id_tag, strong_equality>;

/// \brief UE-specific information related to PUCCH configuration.
struct ue_pucch_config {
  pucch_resource_set_config_id res_set_cfg_id;
  pucch_sr_resource_id         sr_res_id;
  unsigned                     sr_offset;

  bool operator==(const ue_pucch_config& other) const
  {
    return res_set_cfg_id == other.res_set_cfg_id and sr_res_id == other.sr_res_id and sr_offset == other.sr_offset;
  }
};

/// \brief UE-specific information related to periodic CSI reporting configuration.
struct ue_periodic_csi_config {
  pucch_csi_resource_id pucch_res_id;
  unsigned              offset;

  bool operator==(const ue_periodic_csi_config& other) const
  {
    return pucch_res_id == other.pucch_res_id and offset == other.offset;
  }
};

struct ue_pusch_config {
  tx_scheme_codebook tx_cfg;
  /// Whether to enable TP?

  bool operator==(const ue_pusch_config& other) const { return tx_cfg == other.tx_cfg; }
};

struct ue_srs_config {
  srs_config::srs_resource::nof_srs_ports nof_ports;
  /// Offset (periodicity is common).

  bool operator==(const ue_srs_config& other) const { return nof_ports == other.nof_ports; }
};

/// \brief UE-specific information of an UL BWP configuration.
///
/// This structure contains all the UL BWP parameters that may vary per UE. Together with \ref
/// cell_uplink_bwp_config, it defines the complete configuration of a BWP for a specific UE.
///
/// \remark Only meaningful when associated to its \ref cell_uplink_bwp_config.
struct ue_uplink_bwp_config {
  ue_pucch_config pucch;
  // Only present if periodic CSI reporting is enabled in the cell.
  std::optional<ue_periodic_csi_config> periodic_csi_report;
  ue_pusch_config                       pusch;
  ue_srs_config                         srs;

  bool operator==(const ue_uplink_bwp_config& other) const
  {
    return pucch == other.pucch and periodic_csi_report == other.periodic_csi_report and pusch == other.pusch and
           srs == other.srs;
  }
};

struct ue_bwp_config {
  ue_uplink_bwp_config ul;
};

} // namespace ocudu
