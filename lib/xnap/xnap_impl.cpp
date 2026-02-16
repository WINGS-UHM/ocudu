/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xnap_impl.h"
#include "log_helpers.h"
#include "procedures/xn_setup_procedure.h"
#include "procedures/xn_setup_procedure_asn1_helpers.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/xnap/xnap_message.h"

using namespace ocudu;
using namespace asn1::xnap;
using namespace ocucp;

xnap_impl::xnap_impl(const xnap_configuration& xnap_cfg_, xnc_connection_gateway& xnc_gw_, task_executor& ctrl_exec_) :
  logger(ocudulog::fetch_basic_logger("XNAP")), xnap_cfg(xnap_cfg_), xnc_gw(xnc_gw_), ctrl_exec(ctrl_exec_)
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

void xnap_impl::handle_successful_outcome(const successful_outcome_s& msg)
{
  // TODO.
}

void xnap_impl::handle_unsuccessful_outcome(const unsuccessful_outcome_s& msg)
{
  // TODO.
}

// TODO Implement XN setup procedure.
async_task<void> xnap_impl::handle_xn_setup_request_required()
{
  return launch_async<xn_setup_procedure>(xnap_cfg, xnc_gw, logger);
}

void xnap_impl::handle_xn_setup_request(const xn_setup_request_s& msg)
{
  // TODO check XN setup request is a valid message.

  xnap_message xn_setup_resp = generate_asn1_xn_setup_response(xnap_cfg);

  if (not tx_notifier.on_new_message(xn_setup_resp)) {
    logger.error("Failed to send XN Setup Response. Cause: no SCTP association available");
  }
}
