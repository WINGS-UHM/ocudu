// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ngap_asn1_converters.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/ngap/ngap_ies.h"
#include "ocudu/asn1/ngap/ngap_pdu_contents.h"
#include <string>

namespace ocudu::ocucp {

static inline error_type<std::pair<asn1::ngap::cause_c, std::string>>
validate_ng_setup_response(const asn1::ngap::ng_setup_resp_s& response)
{
  asn1::ngap::cause_c cause;
  cause.set_protocol().value = asn1::ngap::cause_protocol_opts::unspecified;

  if (response->served_guami_list.size() == 0) {
    return make_unexpected(std::make_pair(cause, "Served GUAMI list is empty"));
  }
  for (const auto& served_guami_item : response->served_guami_list) {
    if (!asn1_to_guami(served_guami_item.guami).has_value()) {
      return make_unexpected(std::make_pair(cause, "Invalid GUAMI in served GUAMI list"));
    }
  }

  if (response->plmn_support_list.size() == 0) {
    return make_unexpected(std::make_pair(cause, "PLMN support list is empty"));
  }
  for (const auto& plmn_support_item : response->plmn_support_list) {
    for (const auto& slice_support_item : plmn_support_item.slice_support_list) {
      if (slice_support_item.s_nssai.sd_present) {
        if (!slice_differentiator::create(slice_support_item.s_nssai.sd.to_number()).has_value()) {
          return make_unexpected(std::make_pair(cause, "Invalid SD in slice support item"));
        }
      }
    }
  }

  return {};
}

} // namespace ocudu::ocucp
