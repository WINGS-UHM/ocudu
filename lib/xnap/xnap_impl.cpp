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
#include "ocudu/xnap/xnap_message.h"

using namespace ocudu;
using namespace asn1::xnap;
using namespace ocucp;

xnap_impl::xnap_impl(const xnap_configuration& xnap_cfg_, task_executor& ctrl_exec_) :
  logger(ocudulog::fetch_basic_logger("XNAP")), xnap_cfg(xnap_cfg_), ctrl_exec(ctrl_exec_)
{
}

// Note: For fwd declaration of member types, dtor cannot be trivial.
xnap_impl::~xnap_impl() {}

void xnap_impl::handle_message(const xnap_message& msg)
{
  // Run NGAP protocols in Control executor.
  if (not ctrl_exec.execute([this, msg]() {
        // TODO log RX PDU.
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
  return {};
}
