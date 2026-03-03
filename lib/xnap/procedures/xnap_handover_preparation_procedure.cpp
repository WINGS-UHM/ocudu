// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_handover_preparation_procedure.h"
#include "../xnap_asn1_converters.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/xnap/xnap_message.h"
#include "ocudu/xnap/xnap_types.h"

using namespace ocudu;
using namespace ocucp;
using namespace asn1::xnap;

xnap_handover_preparation_procedure::xnap_handover_preparation_procedure(
    const xnap_handover_preparation_request& request_,
    const xnap_ue_id_t&                      ue_id_,
    xnap_message_notifier&                   xnc_notifier_,
    xnap_cu_cp_notifier&                     cu_cp_notifier_,
    protocol_transaction_event_source<asn1::xnap::ho_request_ack_s, asn1::xnap::ho_prep_fail_s>&
                    handover_preparation_outcome_,
    timer_factory   timers,
    xnap_ue_logger& logger_) :
  request(request_),
  ue_id(ue_id_),
  xnc_notifier(xnc_notifier_),
  cu_cp_notifier(cu_cp_notifier_),
  handover_preparation_outcome(handover_preparation_outcome_),
  logger(logger_),
  txn_reloc_prep_timer(timers.create_timer())
{
}

void xnap_handover_preparation_procedure::operator()(coro_context<async_task<xnap_handover_preparation_response>>& ctx)
{
  CORO_BEGIN(ctx);
  logger.log_debug("\"{}\" started...", name());

  if (ue_id == xnap_ue_id_t::invalid) {
    logger.log_error("\"{}\" failed. Cause: Invalid XNAP UE ID", name());
    CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
  }

  if (request.pdu_sessions.empty() || request.pdu_sessions.begin()->second.drbs.empty()) {
    logger.log_error("\"{}\" failed. Cause: {}",
                     name(),
                     request.pdu_sessions.empty() ? fmt::format("PDU session list is empty")
                                                  : fmt::format("DRB list is empty"));
    CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
  }

  // Subscribe to respective publisher to receive HANDOVER REQUEST ACK/HANDOVER PREPARATION FAILURE message.
  transaction_sink.subscribe_to(handover_preparation_outcome, txn_reloc_prep_ms);

  // Send Handover Request to XN-C peer.
  if (!send_handover_request()) {
    logger.log_warning("\"{}\" failed. Cause: Could not send Handover Request", name());
    CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
  }

  CORO_AWAIT(transaction_sink);

  if (!transaction_sink.successful()) {
    if (transaction_sink.timeout_expired()) {
      logger.log_warning(
          "\"{}\" failed. Cause: Timeout receiving Handover Request ACK/Handover Preparation Failure after {}ms",
          name(),
          txn_reloc_prep_ms.count());
      // Initialize Handover Cancellation procedure.
      if (!send_handover_cancel()) {
        logger.log_warning("\"{}\" failed. Cause: Could not send Handover Cancel", name());
        CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
      }

      CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
    }

    if (transaction_sink.failed()) {
      logger.log_warning("\"{}\" failed. Cause: Received Handover Preparation Failure", name());
      CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
    }
  }

  // Forward RRC Handover Command to DU Processor.
  CORO_AWAIT_VALUE(
      rrc_reconfig_success,
      cu_cp_notifier.on_new_rrc_handover_command(
          request.ue_index, transaction_sink.response()->target2_source_ng_ra_nnode_transp_container.copy()));
  if (!rrc_reconfig_success) {
    logger.log_warning("\"{}\" failed. Cause: Received invalid Handover Command", name());
    CORO_EARLY_RETURN(xnap_handover_preparation_response{false});
  }

  logger.log_debug("\"{}\" finished successfully", name());

  // Forward procedure result to DU manager.
  CORO_RETURN(xnap_handover_preparation_response{true});
}

bool xnap_handover_preparation_procedure::send_handover_request()
{
  xnap_message msg = {};
  // Set XNAP PDU contents.
  msg.pdu.set_init_msg();
  msg.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_HO_PREP);
  ho_request_s& ho_request = msg.pdu.init_msg().value.ho_request();

  // Fill XNAP UE ID.
  ho_request->source_ng_ra_nnode_ue_xn_ap_id = xnap_ue_id_to_uint(ue_id);

  // Fill cause.
  ho_request->cause.set_radio_network();
  ho_request->cause.radio_network() = cause_radio_network_layer_opts::ho_desirable_for_radio_reasons;

  // Fill target cell global ID.
  ho_request->target_cell_global_id.set_nr() = cgi_to_asn1(request.nr_cgi);

  // Fill GUAMI.
  ho_request->guami = guami_to_asn1(request.guami);

  // Fill Context information.
  auto& ue_context_info = ho_request->ue_context_info_ho_request;
  // > Fill NG-C UE associated signalling reference.
  ue_context_info.ng_c_ue_ref = request.amf_ue_id;
  // > Fill AMF address.
  ue_context_info.cp_tnl_info_source.set_endpoint_ip_address();
  ue_context_info.cp_tnl_info_source.endpoint_ip_address().from_string(request.amf_addr.to_bitstring());
  // > Fill UE security capabilities.
  auto& sec_cap                              = ue_context_info.ue_security_cap;
  sec_cap.nr_encyption_algorithms            = supported_algorithms_to_asn1(request.sec_ctxt.supported_enc_algos);
  sec_cap.nr_integrity_protection_algorithms = supported_algorithms_to_asn1(request.sec_ctxt.supported_int_algos);
  // > Fill AS security information.
  ue_context_info.security_info.key_ng_ran_star = key_to_asn1(request.sec_ctxt.k);
  ue_context_info.security_info.ncc             = request.sec_ctxt.ncc;
  // > Fill UE aggregated maximum bit rate.
  ue_context_info.ue_ambr.dl_ue_ambr = request.aggregate_maximum_bit_rate_dl;
  ue_context_info.ue_ambr.ul_ue_ambr = request.aggregate_maximum_bit_rate_ul;
  // > Fill PDU session resource setup list.
  fill_asn1_pdu_session_res_list(ue_context_info.pdu_session_res_to_be_setup_list);
  // > Fill RRC container (containing HandoverPreparationInformation).
  ue_context_info.rrc_context = cu_cp_notifier.on_handover_preparation_message_required(request.ue_index);

  // Forward message to XN-C peer.
  if (!xnc_notifier.on_new_message(msg)) {
    logger.log_warning("XN-C notifier is not set. Cannot send Handover Request");
    return false;
  }

  // TODO: Notify the CU-CP about the transmission of a handover request.

  return true;
}

bool xnap_handover_preparation_procedure::send_handover_cancel()
{
  xnap_message msg = {};
  // Set XNAP PDU contents.
  msg.pdu.set_init_msg();
  msg.pdu.init_msg().load_info_obj(ASN1_XNAP_ID_HO_CANCEL);
  ho_cancel_s& ho_cancel = msg.pdu.init_msg().value.ho_cancel();

  ho_cancel->source_ng_ra_nnode_ue_xn_ap_id = xnap_ue_id_to_uint(ue_id);

  ho_cancel->cause.set_radio_network();
  ho_cancel->cause.set_radio_network() = cause_radio_network_layer_opts::txn_relo_cprep_expiry;

  // Forward message to XN-C peer.
  if (!xnc_notifier.on_new_message(msg)) {
    logger.log_warning("XN-C notifier is not set. Cannot send Handover Cancel");
    return false;
  }

  return true;
}

void xnap_handover_preparation_procedure::fill_asn1_pdu_session_res_list(
    pdu_session_res_to_be_setup_list_l& pdu_session_res_list)
{
  for (const auto& [pid, pdu_session_ctxt] : request.pdu_sessions) {
    pdu_session_res_to_be_setup_item_s pdu_session_item;
    pdu_session_item.pdu_session_id = pdu_session_id_to_uint(pid);

    // TODO: move pdu session specific members to pdu session context.
    // For now the PDU session specific information is extracted from the first DRB of the PDU session context.
    const auto& session_ctxt = pdu_session_ctxt.drbs.begin()->second;
    pdu_session_item.s_nssai = s_nssai_to_asn1(session_ctxt.s_nssai);
    pdu_session_item.ul_ng_u_tnl_at_up_f.set_gtp_tunnel();
    pdu_session_item.ul_ng_u_tnl_at_up_f.gtp_tunnel().gtp_teid.from_number(
        session_ctxt.ul_up_tnl_info_to_be_setup_list.begin()->gtp_teid.value());
    pdu_session_item.ul_ng_u_tnl_at_up_f.gtp_tunnel().tnl_address.from_string(
        session_ctxt.ul_up_tnl_info_to_be_setup_list.begin()->tp_address.to_bitstring());

    // Fill PDU session type.
    pdu_session_item.pdu_session_type = pdu_session_type_to_asn1(pdu_session_ctxt.type);

    // Iterate over all DRBs of the PDU session and collect all QoS flows.
    for (const auto& [drb_id, drb_ctxt] : pdu_session_ctxt.drbs) {
      for (const auto& [qfi, qos_flow] : drb_ctxt.qos_flows) {
        qos_flows_to_be_setup_item_s qos_flow_setup_item = {};
        // Set QFI.
        qos_flow_setup_item.qfi = qos_flow_id_to_uint(qfi);
        // Fill QoS flow level QoS parameters.
        qos_flow_setup_item.qos_flow_level_qos_params = fill_asn1_qos_flow_info_item(qos_flow.qos_params);
        pdu_session_item.qos_flows_to_be_setup_list.push_back(qos_flow_setup_item);
      }
    }

    pdu_session_res_list.push_back(pdu_session_item);
  }
}
