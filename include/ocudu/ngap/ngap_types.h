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

#include "ocudu/ran/plmn_identity.h"

namespace ocudu::ocucp {

/// \brief AMF_UE_ID (non ASN1 type of AMF_UE_NGAP_ID) used to identify the UE in the AMF.
/// \remark See TS 38.413 Section 9.3.3.1: AMF_UE_NGAP_ID valid values: (0..2^40-1)
constexpr uint64_t MAX_NOF_AMF_UES = ((uint64_t)1 << 40);
enum class amf_ue_id_t : uint64_t { min = 0, max = MAX_NOF_AMF_UES - 1, invalid = 0x1ffffffffff };

/// Convert AMF_UE_ID type to integer.
constexpr uint64_t amf_ue_id_to_uint(amf_ue_id_t id)
{
  return static_cast<uint64_t>(id);
}

/// Convert integer to AMF_UE_ID type.
constexpr amf_ue_id_t uint_to_amf_ue_id(std::underlying_type_t<amf_ue_id_t> id)
{
  return static_cast<amf_ue_id_t>(id);
}

// Globally unique AMF identifier.
struct guami_t {
  plmn_identity plmn = plmn_identity::test_value();
  uint16_t      amf_set_id;
  uint8_t       amf_pointer;
  uint8_t       amf_region_id;
};

struct ngap_ue_aggr_max_bit_rate {
  uint64_t ue_aggr_max_bit_rate_dl;
  uint64_t ue_aggr_max_bit_rate_ul;
};

enum class ngap_rrc_inactive_transition_report_request : uint8_t {
  // Send RRC Inactive Transition Reports when the UE enters or leaves RRC_INACTIVE state.
  subsequent_state_transition_report = 0,
  // Send one single RRC Inactive Transition Report when UE transitions from RRC_INACTIVE to RRC_IDLE.
  single_rrc_connected_state_report,
  // Stop sending RRC Inactive Transition Reports.
  cancel_report
};

} // namespace ocudu::ocucp
