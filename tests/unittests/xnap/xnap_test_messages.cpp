// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_test_messages.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap_ies.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/xnap/xnap_message.h"
#include "ocudu/xnap/xnap_types.h"

using namespace ocudu;
using namespace ocucp;
using namespace asn1::xnap;

xnap_message ocudu::ocucp::generate_handover_preparation_failure(xnap_ue_id_t xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_unsuccessful_outcome();
  xnap_msg.pdu.unsuccessful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  auto& ho_prep_fail = xnap_msg.pdu.unsuccessful_outcome().value.ho_prep_fail();

  ho_prep_fail->source_ng_ra_nnode_ue_xn_ap_id = xnap_ue_id_to_uint(xnap_ue_id);

  // Fill cause.
  ho_prep_fail->cause.set_radio_network() = asn1::xnap::cause_radio_network_layer_opts::options::unspecified;

  return xnap_msg;
}

xnap_message ocudu::ocucp::generate_handover_request_ack(xnap_ue_id_t source_xnap_ue_id, xnap_ue_id_t target_xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_successful_outcome();
  xnap_msg.pdu.successful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  auto& ho_request_ack = xnap_msg.pdu.successful_outcome().value.ho_request_ack();

  ho_request_ack->source_ng_ra_nnode_ue_xn_ap_id = xnap_ue_id_to_uint(source_xnap_ue_id);
  ho_request_ack->target_ng_ra_nnode_ue_xn_ap_id = xnap_ue_id_to_uint(target_xnap_ue_id);

  // Fill target to source ng ran node transparent container.
  // Create RRC container.
  byte_buffer rrc_container =
      make_byte_buffer(
          "081a115568220201204550001e1004bcc012121600020509a0000193f7c7000000243434840be2e0260030258380f80408d078100009"
          "39dc601349798002692f120200046402051320c6b6c6bb003704020000080800041a235246c013497890000023271adb19127c058332"
          "55ff8092748837146e30dc71b9637dfab6387580221603400c162300e0102908024985950001ff000000000306e10840003c02ca0041"
          "8000001034c080a28500071c48000133557c841c001040c2050c1c9c48a163068e1e408800004280004005a8000864428000c645a800"
          "10024280014025a8001862428001c625a800200842800240c8200a0320902c0c8280c0320b0340c8300e0320d03c0c83810162080440"
          "e829024b92a4a1814388e8acf1379340e9041e2efc0c10e0000001c7feb311aa6ab940b000010cbb00000000000000000008422b5514"
          "011c00401020800388402710038082042000710804e10070204104000e21009c200e0608108001c420138601c10104100038840270c0"
          "020000002086020406080706800071c40000002004000806000809002200a60000231002271c00600040")
          .value();
  ho_request_ack->target2_source_ng_ra_nnode_transp_container = std::move(rrc_container);

  return xnap_msg;
}
