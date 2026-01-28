/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ngap_handover_resource_allocation_procedure.h"
#include "../ngap_asn1_helpers.h"
#include "ocudu/asn1/ngap/common.h"
#include "ocudu/ngap/ngap_message.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::ngap;

ngap_handover_resource_allocation_procedure::ngap_handover_resource_allocation_procedure(
    const ngap_handover_request& request_,
    const amf_ue_id_t            amf_ue_id_,
    ngap_ue_context_list&        ue_ctxt_list_,
    ngap_cu_cp_notifier&         cu_cp_notifier_,
    ngap_message_notifier&       amf_notifier_,
    ocudulog::basic_logger&      logger_) :
  request(request_),
  amf_ue_id(amf_ue_id_),
  ue_ctxt_list(ue_ctxt_list_),
  cu_cp_notifier(cu_cp_notifier_),
  amf_notifier(amf_notifier_),
  logger(logger_)
{
}

void ngap_handover_resource_allocation_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", request.ue_index, name());

  // Notify DU repository about handover request and await request ack.
  CORO_AWAIT_VALUE(response, cu_cp_notifier.on_ngap_handover_request(request));

  if (response.success) {
    // Create NGAP UE.
    if (create_ngap_ue(response.ue_index)) {
      // Update UE with AMF UE ID.
      ngap_ue_context& ue_ctxt = ue_ctxt_list[response.ue_index];
      ue_ctxt_list.update_amf_ue_id(ue_ctxt.ue_ids.ran_ue_id, amf_ue_id);

      // Store Core Network Assist Info for Inactive if present.
      if (request.core_network_assist_info_for_inactive.has_value()) {
        ue_ctxt.core_network_assist_info_for_inactive = request.core_network_assist_info_for_inactive.value();
      }

      // Store RRC inactive transition report request if present.
      if (request.rrc_inactive_transition_report_request.has_value()) {
        ue_ctxt.rrc_inactive_transition_report_request = request.rrc_inactive_transition_report_request.value();
      }

      if (send_handover_request_ack(ue_ctxt.ue_ids.ue_index, ue_ctxt.ue_ids.ran_ue_id)) {
        logger.debug("ue={}: \"{}\" finished successfully", response.ue_index, name());
      } else {
        logger.debug("ue={}: \"{}\" failed", response.ue_index, name());
      }
    } else {
      send_handover_failure();
      logger.debug("ue={}: \"{}\" failed", response.ue_index, name());
      CORO_EARLY_RETURN();
    }
  } else {
    send_handover_failure();
    logger.debug("ue={}: \"{}\" failed", response.ue_index, name());
    CORO_EARLY_RETURN();
  }

  // Notify CU-CP to await the RRC Reconfiguration Complete and DL Status Transfer.
  cu_cp_notifier.on_n2_handover_execution(response.ue_index);

  CORO_RETURN();
}

bool ngap_handover_resource_allocation_procedure::create_ngap_ue(ue_index_t ue_index)
{
  // Create NGAP UE.
  // Allocate RAN-UE-ID.
  ran_ue_id_t ran_ue_id = ue_ctxt_list.allocate_ran_ue_id();
  if (ran_ue_id == ran_ue_id_t::invalid) {
    logger.error("ue={}: No RAN-UE-ID available", ue_index);
    return false;
  }

  // Notify CU-CP about creation of NGAP UE.
  ngap_cu_cp_ue_notifier* ue_notifier = cu_cp_notifier.on_new_ngap_ue(ue_index);
  if (ue_notifier == nullptr) {
    logger.error("ue={}: Failed to create UE", ue_index);
    // Remove created UE context.
    ue_ctxt_list.remove_ue_context(ue_index);
    return false;
  }

  // Create UE context and store it.
  ue_ctxt_list.add_ue(ue_index, ran_ue_id, *ue_notifier);

  ue_ctxt_list[ue_index].logger.log_debug("Created UE");

  return true;
}

bool ngap_handover_resource_allocation_procedure::send_handover_request_ack(ue_index_t ue_index, ran_ue_id_t ran_ue_id)
{
  ngap_message ngap_msg;

  ngap_msg.pdu.set_successful_outcome();
  ngap_msg.pdu.successful_outcome().load_info_obj(ASN1_NGAP_ID_HO_RES_ALLOC);

  fill_asn1_handover_resource_allocation_response(ngap_msg.pdu.successful_outcome().value.ho_request_ack(), response);

  auto& ho_request_ack           = ngap_msg.pdu.successful_outcome().value.ho_request_ack();
  ho_request_ack->amf_ue_ngap_id = amf_ue_id_to_uint(amf_ue_id);
  ho_request_ack->ran_ue_ngap_id = ran_ue_id_to_uint(ran_ue_id);

  // Forward message to AMF.
  if (!amf_notifier.on_new_message(ngap_msg)) {
    logger.warning("AMF notifier is not set. Cannot send HandoverRequestAck");
    return false;
  }

  return true;
}

void ngap_handover_resource_allocation_procedure::send_handover_failure()
{
  ngap_message ngap_msg;
  ngap_msg.pdu.set_unsuccessful_outcome();
  ngap_msg.pdu.unsuccessful_outcome().load_info_obj(ASN1_NGAP_ID_HO_RES_ALLOC);

  fill_asn1_handover_resource_allocation_response(ngap_msg.pdu.unsuccessful_outcome().value.ho_fail(), response);

  auto& ho_fail           = ngap_msg.pdu.unsuccessful_outcome().value.ho_fail();
  ho_fail->amf_ue_ngap_id = amf_ue_id_to_uint(amf_ue_id);

  // Forward message to AMF.

  if (!amf_notifier.on_new_message(ngap_msg)) {
    logger.warning("AMF notifier is not set. Cannot send HandoverFailure");
    return;
  }
}
