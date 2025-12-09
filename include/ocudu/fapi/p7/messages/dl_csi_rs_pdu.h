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

#include "ocudu/fapi/p7/messages/power_control_offset_ss.h"
#include "ocudu/fapi/p7/messages/tx_precoding_and_beamforming_pdu.h"
#include "ocudu/ran/csi_rs/csi_rs_types.h"
#include "ocudu/ran/csi_rs/frequency_allocation_type.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace fapi {

/// CSI-RS maintenance parameters added in FAPIv3.
struct dl_csi_rs_maintenance_v3 {
  uint16_t csi_rs_pdu_index;
};

/// Downlink CSI-RS PDU information.
struct dl_csi_rs_pdu {
  subcarrier_spacing                scs;
  cyclic_prefix                     cp;
  uint16_t                          start_rb;
  uint16_t                          num_rbs;
  csi_rs_type                       type;
  uint8_t                           row;
  csi_rs::freq_allocation_mask_type freq_domain;
  uint8_t                           symb_L0;
  uint8_t                           symb_L1;
  csi_rs_cdm_type                   cdm_type;
  csi_rs_freq_density_type          freq_density;
  uint16_t                          scramb_id;
  int                               power_control_offset_profile_nr;
  power_control_offset_ss           power_control_offset_ss_profile_nr;
  tx_precoding_and_beamforming_pdu  precoding_and_beamforming;
  dl_csi_rs_maintenance_v3          csi_rs_maintenance_v3;
  //: TODO: csi params v4
  // Vendor specific parameters.
  uint16_t bwp_size;
  uint16_t bwp_start;
};

} // namespace fapi
} // namespace ocudu
