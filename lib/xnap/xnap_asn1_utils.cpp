// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_asn1_utils.h"
#include "ocudu/asn1/xnap/xnap_ies.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

const char* ocudu::ocucp::asn1_utils::get_cause_str(const asn1::xnap::cause_c& cause)
{
  using namespace asn1::xnap;
  switch (cause.type()) {
    case cause_c::types_opts::radio_network:
      return cause.radio_network().to_string();
    case cause_c::types_opts::transport:
      return cause.transport().to_string();
    case cause_c::types_opts::protocol:
      return cause.protocol().to_string();
    case cause_c::types_opts::misc:
      return cause.misc().to_string();
    default:
      break;
  }
  return "unknown";
}

const char* ocudu::ocucp::asn1_utils::get_message_type_str(const asn1::xnap::xn_ap_pdu_c& pdu)
{
  switch (pdu.type().value) {
    case xn_ap_pdu_c::types_opts::init_msg:
      return pdu.init_msg().value.type().to_string();
    case xn_ap_pdu_c::types_opts::successful_outcome:
      return pdu.successful_outcome().value.type().to_string();
    case xn_ap_pdu_c::types_opts::unsuccessful_outcome:
      return pdu.unsuccessful_outcome().value.type().to_string();
    default:
      break;
  }
  report_fatal_error("Invalid XNAP PDU type \"{}\"", pdu.type().to_string());
}
