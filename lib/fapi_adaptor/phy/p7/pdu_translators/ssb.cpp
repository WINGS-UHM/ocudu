/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ssb.h"
#include "ocudu/ocuduvec/bit.h"
#include "ocudu/ran/ssb/pbch_mib_pack.h"

using namespace ocudu;
using namespace fapi_adaptor;

/// Unpacks the contents of the BCH payload.
static void unpack_bch_payload(span<uint8_t> dest, const fapi::dl_ssb_pdu& fapi_pdu)
{
  ocuduvec::bit_unpack(dest, fapi_pdu.bch_payload, dest.size());
}

/// Returns the coefficient \f$\beta_{PSS}\f$ from the given SSB PDU (see TS38.213, Section 4.1).
static float convert_to_beta_pss(const fapi::dl_ssb_pdu& fapi_pdu)
{
  switch (fapi_pdu.beta_pss_profile_nr) {
    case fapi::beta_pss_profile_type::dB_0:
      return 0.F;
    case fapi::beta_pss_profile_type::dB_3:
      return 3.F;
    default:
      // NOTE: Unreachable code as the FAPI message should have been validated.
      ocudu_assert(0, "Invalid beta PSS profile");
      return 0;
  }
}

void ocudu::fapi_adaptor::convert_ssb_fapi_to_phy(ssb_processor::pdu_t&   proc_pdu,
                                                  const fapi::dl_ssb_pdu& fapi_pdu,
                                                  slot_point              slot,
                                                  subcarrier_spacing      scs_common)
{
  proc_pdu.slot              = slot;
  proc_pdu.phys_cell_id      = fapi_pdu.phys_cell_id;
  proc_pdu.beta_pss          = convert_to_beta_pss(fapi_pdu);
  proc_pdu.ssb_idx           = fapi_pdu.ssb_block_index;
  proc_pdu.L_max             = fapi_pdu.L_max;
  proc_pdu.subcarrier_offset = fapi_pdu.subcarrier_offset;
  proc_pdu.offset_to_pointA  = fapi_pdu.ssb_offset_pointA;
  proc_pdu.pattern_case      = fapi_pdu.case_type;
  proc_pdu.common_scs        = scs_common;

  unpack_bch_payload(proc_pdu.mib_payload, fapi_pdu);

  // Use only a single port for SSB.
  proc_pdu.ports = {0};
}
