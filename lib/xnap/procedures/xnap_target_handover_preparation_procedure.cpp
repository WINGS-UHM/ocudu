// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_target_handover_preparation_procedure.h"
#include "xn_handover_asn1_helpers.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/xnap/xnap_message.h"
#include <variant>

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

xnap_target_handover_preparation_procedure::xnap_target_handover_preparation_procedure(
    const xnap_handover_request& request_,
    xnc_peer_index_t             xnc_index_,
    const peer_xnap_ue_id_t      target_xnap_ue_id_,
    xnap_ue_context_list&        ue_ctxt_list_,
    xnap_cu_cp_notifier&         cu_cp_notifier_,
    xnap_message_notifier&       tx_notifier_,
    ocudulog::basic_logger&      logger_) :
  request(request_),
  xnc_index(xnc_index_),
  target_xnap_ue_id(target_xnap_ue_id_),
  ue_ctxt_list(ue_ctxt_list_),
  cu_cp_notifier(cu_cp_notifier_),
  tx_notifier(tx_notifier_),
  logger(logger_)
{
}

void xnap_target_handover_preparation_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", request.ue_index, name());

  // Notify DU repository about handover request and await request ack.
  CORO_AWAIT_VALUE(response, cu_cp_notifier.on_xnap_handover_request(request));

  if (std::holds_alternative<cu_cp_handover_request_failure>(response)) {
    const auto& ho_fail = std::get<cu_cp_handover_request_failure>(response);
    send_handover_preparation_failure(ho_fail);
    logger.debug("ue={}: \"{}\" failed", ho_fail.ue_index, name());
    CORO_EARLY_RETURN();
  }

  const auto& ho_ack = std::get<cu_cp_handover_request_ack>(response);

  // Create XNAP UE.
  if (create_xnap_ue(ho_ack.ue_index)) {
    // Update UE with PEER XNAP UE ID.
    xnap_ue_context& ue_ctxt = ue_ctxt_list[ho_ack.ue_index];
    ue_ctxt_list.update_peer_xnap_ue_id(ue_ctxt.ue_ids.local_xnap_ue_id, target_xnap_ue_id);

    if (send_handover_request_ack(ue_ctxt.ue_ids.ue_index, ue_ctxt.ue_ids.local_xnap_ue_id, ho_ack)) {
      logger.debug("ue={}: \"{}\" finished successfully", ho_ack.ue_index, name());
    } else {
      logger.debug("ue={}: \"{}\" failed", ho_ack.ue_index, name());
    }
  } else {
    send_handover_preparation_failure(
        cu_cp_handover_request_failure{.ue_index = request.ue_index, .cause = xnap_cause_misc_t::unspecified});
    logger.debug("ue={}: \"{}\" failed", request.ue_index, name());
    CORO_EARLY_RETURN();
  }

  if (!prepare_execution_context(ho_ack)) {
    send_handover_preparation_failure(cu_cp_handover_request_failure{
        .ue_index = request.ue_index, .cause = xnap_cause_misc_t::not_enough_user_plane_processing_res});
    logger.debug("ue={}: \"{}\" failed", request.ue_index, name());
    CORO_EARLY_RETURN();
  }

  // Notify CU-CP to await the RRC Reconfiguration Complete and SN Status Transfer.
  cu_cp_notifier.on_xn_handover_execution(request.ue_index, execution_context);

  CORO_RETURN();
}

bool xnap_target_handover_preparation_procedure::create_xnap_ue(ue_index_t ue_index)
{
  // Create XNAP UE.
  // Allocate local XNAP UE ID.
  local_xnap_ue_id_t local_xnap_ue_id = ue_ctxt_list.allocate_local_xnap_ue_id();
  if (local_xnap_ue_id == local_xnap_ue_id_t::invalid) {
    logger.error("ue={}: No local XNAP UE ID available", ue_index);
    return false;
  }

  // Create UE context and store it.
  ue_ctxt_list.add_ue(ue_index, local_xnap_ue_id);

  ue_ctxt_list[ue_index].logger.log_debug("Created UE");

  return true;
}

bool xnap_target_handover_preparation_procedure::send_handover_request_ack(ue_index_t         ue_index,
                                                                           local_xnap_ue_id_t local_xnap_ue_id,
                                                                           const cu_cp_handover_request_ack& ho_ack)
{
  xnap_message xnap_msg;

  xnap_msg.pdu.set_successful_outcome();
  xnap_msg.pdu.successful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  handover_request_ack_to_asn1(xnap_msg.pdu.successful_outcome().value.ho_request_ack(), ho_ack);

  auto& ho_request_ack = xnap_msg.pdu.successful_outcome().value.ho_request_ack();
  // Set UE IDs. Note that this is the target CU-CP, so the source XNAP UE ID in the request becomes the target XNAP UE
  // ID in the response and vice versa.
  ho_request_ack->source_ng_ra_nnode_ue_xn_ap_id = peer_xnap_ue_id_to_uint(target_xnap_ue_id);
  ho_request_ack->target_ng_ra_nnode_ue_xn_ap_id = local_xnap_ue_id_to_uint(local_xnap_ue_id);

  // Forward message to XN-C peer.
  if (!tx_notifier.on_new_message(xnap_msg)) {
    logger.warning("XN-C association is not set. Cannot send HandoverRequestAck");
    return false;
  }

  return true;
}

void xnap_target_handover_preparation_procedure::send_handover_preparation_failure(
    const cu_cp_handover_request_failure& ho_failure)
{
  xnap_message xnap_msg;
  xnap_msg.pdu.set_unsuccessful_outcome();
  xnap_msg.pdu.unsuccessful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);

  handover_preparation_failure_to_asn1(xnap_msg.pdu.unsuccessful_outcome().value.ho_prep_fail(), ho_failure);

  auto& ho_fail                           = xnap_msg.pdu.unsuccessful_outcome().value.ho_prep_fail();
  ho_fail->source_ng_ra_nnode_ue_xn_ap_id = peer_xnap_ue_id_to_uint(target_xnap_ue_id);

  // Forward message to XN-C peer.
  if (!tx_notifier.on_new_message(xnap_msg)) {
    logger.warning("XN-C association is not set. Cannot send HandoverFailure");
    return;
  }
}

bool xnap_target_handover_preparation_procedure::prepare_execution_context(const cu_cp_handover_request_ack& ho_ack)
{
  execution_context.xnc_index = xnc_index;
  execution_context.ue_index  = ho_ack.ue_index;
  execution_context.amf_ue_id = request.ue_context_info_ho_request.amf_ue_id;
  for (const auto& variant_pdu_session_item : ho_ack.pdu_session_res_admitted_list) {
    if (!std::holds_alternative<cu_cp_xn_pdu_session_res_admitted_item>(variant_pdu_session_item)) {
      return false;
    }
    execution_context.pdu_session_res_admitted_list.push_back(
        std::get<cu_cp_xn_pdu_session_res_admitted_item>(variant_pdu_session_item));
  }
  execution_context.pdu_session_failed_to_setup_list = ho_ack.pdu_session_failed_to_setup_list;
  return true;
}
