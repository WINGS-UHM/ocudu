// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xnap_impl.h"
#include "log_helpers.h"
#include "procedures/xn_setup_procedure.h"
#include "procedures/xn_setup_procedure_asn1_helpers.h"
#include "procedures/xnap_handover_preparation_procedure.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
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

async_task<xnap_handover_preparation_response>
xnap_impl::handle_handover_preparation_request(const xnap_handover_preparation_request& msg)
{
  auto err_function = [](coro_context<async_task<xnap_handover_preparation_response>>& ctx) {
    CORO_BEGIN(ctx);
    CORO_RETURN(xnap_handover_preparation_response{false});
  };

  if (!ue_ctxt_list.contains(msg.ue_index)) {
    // Allocate new XNAP UE context if it doesn't exist.
    xnap_ue_id_t xnap_ue_id = ue_ctxt_list.allocate_xnap_ue_id();
    if (xnap_ue_id == xnap_ue_id_t::invalid) {
      logger.error("Failed to allocate XNAP UE ID for ue={}. Cannot handle HandoverPreparationRequest", msg.ue_index);
      return launch_async(std::move(err_function));
    }
    ue_ctxt_list.add_ue(msg.ue_index, xnap_ue_id);
  }

  xnap_ue_context& ue_ctxt = ue_ctxt_list[msg.ue_index];

  ue_ctxt.logger.log_debug("Starting HO preparation");

  return launch_async<xnap_handover_preparation_procedure>(msg,
                                                           ue_ctxt.ue_ids.xnap_ue_id,
                                                           tx_notifier,
                                                           cu_cp_notifier,
                                                           xn_handover_outcome,
                                                           timer_factory{timers, ctrl_exec},
                                                           ue_ctxt.logger);
}
