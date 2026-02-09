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
#include "ocudu/ran/csi_rs/csi_rs_types.h"
#include "ocudu/ran/csi_rs/frequency_allocation_type.h"
#include "ocudu/ran/cyclic_prefix.h"
#include "ocudu/ran/resource_allocation/rb_interval.h"
#include "ocudu/ran/subcarrier_spacing.h"

namespace ocudu {
namespace fapi {

/// Downlink CSI-RS PDU information.
struct dl_csi_rs_pdu {
  subcarrier_spacing                scs;
  cyclic_prefix                     cp;
  crb_interval                      crbs;
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
  crb_interval                      bwp;
};

} // namespace fapi
} // namespace ocudu
