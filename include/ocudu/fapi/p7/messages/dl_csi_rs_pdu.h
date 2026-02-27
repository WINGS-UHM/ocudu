// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI

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

namespace fmt {
template <>
struct formatter<ocudu::fapi::dl_csi_rs_pdu> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::fapi::dl_csi_rs_pdu& pdu, FormatContext& ctx) const
  {
    if (pdu.type == ocudu::csi_rs_type::CSI_RS_NZP) {
      return format_to(ctx.out(),
                       "\n\t- NZP-CSI-RS crbs={} type={} freq_domain={} row={} symbL0={} symbL1={} cdm_type={} "
                       "freq_density={} scramb_id={}",
                       pdu.crbs,
                       to_string(pdu.type),
                       pdu.freq_domain,
                       pdu.row,
                       pdu.symb_L0,
                       pdu.symb_L1,
                       to_string(pdu.cdm_type),
                       to_string(pdu.freq_density),
                       pdu.scramb_id);
    }

    if (pdu.type == ocudu::csi_rs_type::CSI_RS_ZP) {
      return fmt::format_to(
          ctx.out(), "\n\t- ZP-CSI-RS crbs={} row={} symbL0={} symbL1={}", pdu.crbs, pdu.row, pdu.symb_L0, pdu.symb_L1);
    }

    return ctx.out();
  }
};
} // namespace fmt
