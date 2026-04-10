// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_test_messages.h"
#include "lib/xnap/xnap_asn1_converters.h"
#include "ocudu/asn1/asn1_utils.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap_ies.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/ran/plmn_identity.h"
#include "ocudu/xnap/xnap_message.h"
#include "ocudu/xnap/xnap_types.h"

using namespace ocudu;
using namespace ocucp;
using namespace asn1::xnap;

xnap_message ocudu::ocucp::generate_handover_request(local_xnap_ue_id_t local_xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_init_msg();
  xnap_msg.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  auto& ho_request = xnap_msg.pdu.init_msg().value.ho_request();

  ho_request->source_ng_ra_nnode_ue_xn_ap_id = local_xnap_ue_id_to_uint(local_xnap_ue_id);
  ho_request->cause.set_radio_network() =
      asn1::xnap::cause_radio_network_layer_opts::options::ho_desirable_for_radio_reasons;
  ho_request->target_cell_global_id.set_nr() =
      cgi_to_asn1(nr_cell_global_id_t{plmn_identity::test_value(), nr_cell_identity::create({411, 22}, 0).value()});
  ho_request->guami = guami_to_asn1(
      guami_t{.plmn = plmn_identity::test_value(), .amf_set_id = 1, .amf_pointer = 1, .amf_region_id = 1});
  ho_request->ue_context_info_ho_request.ng_c_ue_ref = 1;

  ho_request->ue_context_info_ho_request.cp_tnl_info_source.set_endpoint_ip_address();
  tla_to_asn1_bitstring(ho_request->ue_context_info_ho_request.cp_tnl_info_source.endpoint_ip_address(),
                        transport_layer_address::create_from_string("127.0.0.1"));
  ho_request->ue_context_info_ho_request.ue_security_cap.nr_encyption_algorithms.from_number(49152);
  ho_request->ue_context_info_ho_request.ue_security_cap.nr_integrity_protection_algorithms.from_number(49152);
  ho_request->ue_context_info_ho_request.security_info.key_ng_ran_star.from_string(
      "1111111000001101100111110001011010001110110010111010001100110111100111011000110110011010000110000011000010111000"
      "0010001100001010000001111111100000100111101011000011110000110101110010001010001010101000101100101100100000001110"
      "00010001000110001101110101100110");
  ho_request->ue_context_info_ho_request.ue_ambr.dl_ue_ambr = 1000000000;
  ho_request->ue_context_info_ho_request.ue_ambr.ul_ue_ambr = 1000000000;

  pdu_session_res_to_be_setup_item_s pdu_session_item;
  pdu_session_item.pdu_session_id = 1;
  pdu_session_item.s_nssai =
      s_nssai_to_asn1(s_nssai_t{.sst = slice_service_type{1}, .sd = slice_differentiator::create(1).value()});
  up_transport_layer_info_to_asn1(
      pdu_session_item.ul_ng_u_tnl_at_up_f,
      up_transport_layer_info{transport_layer_address::create_from_string("127.0.0.1"), gtpu_teid_t{12345}});
  pdu_session_item.pdu_session_type = pdu_session_type_e::ipv4;

  qos_flows_to_be_setup_item_s qos_flow_item;
  qos_flow_item.qfi = 1;
  qos_flow_item.qos_flow_level_qos_params.qos_characteristics.set_non_dyn();
  qos_flow_item.qos_flow_level_qos_params.qos_characteristics.non_dyn().five_qi = 9;
  qos_flow_item.qos_flow_level_qos_params.alloc_and_retention_prio.pre_emption_cap =
      asn1::xnap::allocand_retention_prio_s::pre_emption_cap_opts::options::shall_not_trigger_preemption;
  qos_flow_item.qos_flow_level_qos_params.alloc_and_retention_prio.pre_emption_vulnerability =
      asn1::xnap::allocand_retention_prio_s::pre_emption_vulnerability_opts::options::not_preemptable;

  pdu_session_item.qos_flows_to_be_setup_list.push_back(qos_flow_item);
  ho_request->ue_context_info_ho_request.pdu_session_res_to_be_setup_list.push_back(pdu_session_item);

  ho_request->ue_context_info_ho_request.rrc_context =
      make_byte_buffer(
          "00217b8680ce811d1960097e360e1317000183f1300098a09a00000020400f13400389a00000000e268208010010134a0f0040000000"
          "00000040000000247001040000259650100400002596500052388008404008010100200400200801052050")
          .value();

  asn1::xnap::last_visited_cell_item_c last_visited_cell;
  last_visited_cell.set_ng_ran_cell() = make_byte_buffer("0000f11000066c0000800000").value();
  ho_request->ue_history_info.push_back(last_visited_cell);

  return xnap_msg;
}

xnap_message ocudu::ocucp::generate_handover_preparation_failure(peer_xnap_ue_id_t peer_xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_unsuccessful_outcome();
  xnap_msg.pdu.unsuccessful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  auto& ho_prep_fail = xnap_msg.pdu.unsuccessful_outcome().value.ho_prep_fail();

  ho_prep_fail->source_ng_ra_nnode_ue_xn_ap_id = peer_xnap_ue_id_to_uint(peer_xnap_ue_id);

  // Fill cause.
  ho_prep_fail->cause.set_radio_network() = asn1::xnap::cause_radio_network_layer_opts::options::unspecified;

  return xnap_msg;
}

xnap_message ocudu::ocucp::generate_handover_request_ack(local_xnap_ue_id_t local_xnap_ue_id,
                                                         peer_xnap_ue_id_t  peer_xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_successful_outcome();
  xnap_msg.pdu.successful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  auto& ho_request_ack = xnap_msg.pdu.successful_outcome().value.ho_request_ack();

  ho_request_ack->source_ng_ra_nnode_ue_xn_ap_id = peer_xnap_ue_id_to_uint(peer_xnap_ue_id);
  ho_request_ack->target_ng_ra_nnode_ue_xn_ap_id = local_xnap_ue_id_to_uint(local_xnap_ue_id);

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

xnap_message ocudu::ocucp::generate_sn_status_transfer(local_xnap_ue_id_t local_xnap_ue_id,
                                                       peer_xnap_ue_id_t  peer_xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_init_msg();
  xnap_msg.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_S_N_STATUS_TRANSFER);

  auto& sn_status_transfer = xnap_msg.pdu.init_msg().value.sn_status_transfer();

  sn_status_transfer->source_ng_ra_nnode_ue_xn_ap_id = local_xnap_ue_id_to_uint(local_xnap_ue_id);
  sn_status_transfer->target_ng_ra_nnode_ue_xn_ap_id = peer_xnap_ue_id_to_uint(peer_xnap_ue_id);

  drbs_subject_to_status_transfer_item_s drb_item;
  drb_item.drb_id = 1;
  drb_item.pdcp_status_transfer_ul.set_pdcp_sn_12bits();
  drb_item.pdcp_status_transfer_dl.set_pdcp_sn_12bits();

  sn_status_transfer->drbs_subject_to_status_transfer_list.push_back(drb_item);

  return xnap_msg;
}

xnap_message ocudu::ocucp::generate_ue_context_release(local_xnap_ue_id_t local_xnap_ue_id,
                                                       peer_xnap_ue_id_t  peer_xnap_ue_id)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_init_msg();
  xnap_msg.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_U_E_CONTEXT_RELEASE);

  auto& ue_context_release = xnap_msg.pdu.init_msg().value.ue_context_release();

  ue_context_release->source_ng_ra_nnode_ue_xn_ap_id = peer_xnap_ue_id_to_uint(peer_xnap_ue_id);
  ue_context_release->target_ng_ra_nnode_ue_xn_ap_id = local_xnap_ue_id_to_uint(local_xnap_ue_id);

  return xnap_msg;
}
