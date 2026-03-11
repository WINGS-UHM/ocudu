// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_impl.h"
#include "log_helpers.h"
#include "procedures/xn_handover_asn1_helpers.h"
#include "procedures/xn_setup_procedure.h"
#include "procedures/xn_setup_procedure_asn1_helpers.h"
#include "procedures/xnap_source_handover_preparation_procedure.h"
#include "procedures/xnap_target_handover_preparation_procedure.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap.h"
#include "ocudu/asn1/xnap/xnap_ies.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/support/async/async_no_op_task.h"
#include "ocudu/xnap/xnap_message.h"

using namespace ocudu;
using namespace asn1::xnap;
using namespace ocucp;

xnap_impl::xnap_impl(const xnap_configuration&              xnap_cfg_,
                     xnap_cu_cp_notifier&                   cu_cp_notifier_,
                     std::unique_ptr<xnap_message_notifier> init_tx_notifier_,
                     timer_manager&                         timers_,
                     task_executor&                         ctrl_exec_) :
  logger(ocudulog::fetch_basic_logger("XNAP")),
  ue_ctxt_list(logger),
  xnap_cfg(xnap_cfg_),
  cu_cp_notifier(cu_cp_notifier_),
  timers(timers_),
  ctrl_exec(ctrl_exec_),
  tx_notifier(std::move(init_tx_notifier_)),
  xn_setup_outcome(timer_factory{timers, ctrl_exec}),
  xn_handover_outcome(timer_factory{timers, ctrl_exec})
{
}

async_task<void> xnap_impl::stop()
{
  // Stop XN setup procedure if in progress.
  xn_setup_outcome.stop();
  return launch_no_op_task();
}

void xnap_impl::handle_message(const xnap_message& msg)
{
  // Run XNAP protocols in Control executor.
  if (not ctrl_exec.execute([this, msg]() {
        log_xnap_pdu(logger, logger.debug.enabled(), true, msg.pdu);
        switch (msg.pdu.type().value) {
          case xn_ap_pdu_c::types_opts::init_msg:
            handle_initiating_message(msg.pdu.init_msg());
            break;
          case xn_ap_pdu_c::types_opts::successful_outcome:
            handle_successful_outcome(msg.pdu.successful_outcome());
            break;
          case xn_ap_pdu_c::types_opts::unsuccessful_outcome:
            handle_unsuccessful_outcome(msg.pdu.unsuccessful_outcome());
            break;
          default:
            logger.error("Invalid PDU type");
            break;
        }
      })) {
    logger.error("Discarding Rx XNAP PDU. Cause: task queue is full");
  }
}

void xnap_impl::handle_initiating_message(const init_msg_s& msg)
{
  switch (msg.value.type().value) {
    case xnap_elem_procs_o::init_msg_c::types_opts::xn_setup_request:
      handle_xn_setup_request(msg.value.xn_setup_request());
      break;
    case xnap_elem_procs_o::init_msg_c::types_opts::ho_request:
      handle_handover_request(msg.value.ho_request());
      break;
    default:
      logger.error("Initiating message of type {} is not supported", msg.value.type().to_string());
  }
}

void xnap_impl::handle_successful_outcome(const successful_outcome_s& outcome)
{
  switch (outcome.value.type().value) {
    case xnap_elem_procs_o::successful_outcome_c::types_opts::xn_setup_resp: {
      xn_setup_outcome.set(outcome.value.xn_setup_resp());
    } break;
    case xnap_elem_procs_o::successful_outcome_c::types_opts::ho_request_ack: {
      xn_handover_outcome.set(outcome.value.ho_request_ack());
    } break;
    default:
      logger.error("Successful outcome of type {} is not supported", outcome.value.type().to_string());
  }
}

void xnap_impl::handle_unsuccessful_outcome(const unsuccessful_outcome_s& outcome)
{
  switch (outcome.value.type().value) {
    case xnap_elem_procs_o::unsuccessful_outcome_c::types_opts::xn_setup_fail: {
      xn_setup_outcome.set(outcome.value.xn_setup_fail());
    } break;
    case xnap_elem_procs_o::unsuccessful_outcome_c::types_opts::ho_prep_fail: {
      xn_handover_outcome.set(outcome.value.ho_prep_fail());
    } break;
    default:
      logger.error("Unsuccessful outcome of type {} is not supported", outcome.value.type().to_string());
  }
}

async_task<bool> xnap_impl::handle_xn_setup_request_required()
{
  return launch_async<xn_setup_procedure>(
      xnap_cfg, tx_notifier, xn_setup_outcome, timer_factory{timers, ctrl_exec}, logger);
}

void xnap_impl::handle_xn_setup_request(const xn_setup_request_s& msg)
{
  // TODO check XN setup request is a valid message.

  xnap_message xn_setup_resp = generate_asn1_xn_setup_response(xnap_cfg);

  if (not tx_notifier.on_new_message(xn_setup_resp)) {
    logger.error("Failed to send XN Setup Response. Cause: no SCTP association available");
  }
}

void xnap_impl::handle_handover_request(const asn1::xnap::ho_request_s& msg)
{
  // Add lambda that generates and transmits Handover Preparation Failure message.
  auto send_handover_failure = [this](uint64_t local_xnap_ue_id, xnap_cause_t cause) {
    xnap_message xnap_msg;
    xnap_msg.pdu.set_unsuccessful_outcome();
    xnap_msg.pdu.unsuccessful_outcome().load_info_obj(ASN1_XNAP_ID_HO_PREP);
    auto& ho_fail                           = xnap_msg.pdu.unsuccessful_outcome().value.ho_prep_fail();
    ho_fail->source_ng_ra_nnode_ue_xn_ap_id = local_xnap_ue_id;
    ho_fail->cause                          = cause_to_asn1(cause);

    if (!tx_notifier.on_new_message(xnap_msg)) {
      logger.warning("XN-C association is not set. Cannot send HandoverFailure");
      return;
    }
    logger.warning("Sending HandoverFailure");
  };

  // Convert Handover Request to common type.
  xnap_handover_request ho_request;
  if (!asn1_to_handover_request(ho_request, msg)) {
    logger.info("Received invalid HandoverRequest");
    send_handover_failure(msg->source_ng_ra_nnode_ue_xn_ap_id,
                          cause_protocol_t::abstract_syntax_error_falsely_constructed_msg);

    return;
  }

  logger.info("HandoverRequest - extracted target cell. plmn={}, target cell_id={}",
              ho_request.nr_cgi.plmn_id,
              ho_request.nr_cgi.nci);

  // Create UE in target cell.
  ho_request.ue_index = cu_cp_notifier.request_new_ue_index_allocation(ho_request.nr_cgi, ho_request.guami.plmn);
  if (ho_request.ue_index == ue_index_t::invalid) {
    logger.debug("Couldn't allocate UE index for handover target cell");
    send_handover_failure(msg->source_ng_ra_nnode_ue_xn_ap_id, xnap_cause_misc_t::not_enough_user_plane_processing_res);
    return;
  }

  // Inititialize security context of target UE.
  if (!cu_cp_notifier.on_handover_request_received(
          ho_request.ue_index, ho_request.guami.plmn, ho_request.ue_context_info_ho_request.security_context)) {
    logger.debug("Failed to initialize security context for UE index {}. Rejecting handover request",
                 ho_request.ue_index);
    send_handover_failure(msg->source_ng_ra_nnode_ue_xn_ap_id, xnap_cause_misc_t::not_enough_user_plane_processing_res);
    return;
  }

  if (!cu_cp_notifier.schedule_async_task(ho_request.ue_index,
                                          launch_async<xnap_target_handover_preparation_procedure>(
                                              ho_request,
                                              uint_to_peer_xnap_ue_id(msg->source_ng_ra_nnode_ue_xn_ap_id),
                                              ue_ctxt_list,
                                              cu_cp_notifier,
                                              tx_notifier,
                                              logger))) {
    logger.debug("Couldn't schedule targer handover preparation procedure");
    send_handover_failure(msg->source_ng_ra_nnode_ue_xn_ap_id, xnap_cause_misc_t::not_enough_user_plane_processing_res);
    return;
  }
}

async_task<xnap_handover_preparation_response>
xnap_impl::handle_handover_request_required(const xnap_handover_request& request)
{
  if (!ue_ctxt_list.contains(request.ue_index)) {
    // Allocate new local XNAP UE context if it doesn't exist.
    local_xnap_ue_id_t local_xnap_ue_id = ue_ctxt_list.allocate_local_xnap_ue_id();
    if (local_xnap_ue_id == local_xnap_ue_id_t::invalid) {
      logger.error("Failed to allocate XNAP UE ID for ue={}. Cannot handle HandoverPreparationRequest",
                   request.ue_index);
      return launch_no_op_task(xnap_handover_preparation_response{false});
    }
    ue_ctxt_list.add_ue(request.ue_index, local_xnap_ue_id);
  }

  xnap_ue_context& ue_ctxt = ue_ctxt_list[request.ue_index];

  ue_ctxt.logger.log_debug("Starting HO source preparation");

  return launch_async<xnap_source_handover_preparation_procedure>(request,
                                                                  ue_ctxt.ue_ids.local_xnap_ue_id,
                                                                  tx_notifier,
                                                                  cu_cp_notifier,
                                                                  xn_handover_outcome,
                                                                  timer_factory{timers, ctrl_exec},
                                                                  ue_ctxt.logger);
}
