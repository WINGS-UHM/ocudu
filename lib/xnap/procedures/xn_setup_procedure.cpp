/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "xn_setup_procedure.h"
#include "xn_setup_procedure_asn1_helpers.h"
#include "ocudu/asn1/xnap/common.h"
#include "ocudu/asn1/xnap/xnap_pdu_contents.h"
#include "ocudu/xnap/gateways/xnc_connection_gateway.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

xn_setup_procedure::xn_setup_procedure(xnap_configuration      xnap_cfg_,
                                       xnc_connection_gateway& xnc_gw_,
                                       ocudulog::basic_logger& logger_) :
  xnap_cfg(xnap_cfg_), xnc_gw(xnc_gw_), logger(logger_)
{
}

void xn_setup_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.info("\"{}\" started...", name());

  // Prepare XN Setup message.
  xn_setup_req = generate_asn1_xn_setup_request(xnap_cfg);

  // Send XN Setup message.
  send_xn_setup_message();

  logger.info("\"{}\" finished successfully", name());

  CORO_RETURN();
}

void xn_setup_procedure::send_xn_setup_message()
{
  if (logger.debug.enabled()) {
    asn1::json_writer js;
    xn_setup_req.pdu.to_json(js);
    logger.debug("Containerized XN Setup PDU: {}", js.to_string());
  }

  // Pack XNAP PDU into SCTP SDU.
  byte_buffer   tx_pdu{byte_buffer::fallback_allocation_tag{}};
  asn1::bit_ref bref(tx_pdu);
  if (xn_setup_req.pdu.pack(bref) != asn1::OCUDUASN_SUCCESS) {
    logger.error("Failed to pack XNAP PDU");
    return;
  }

  logger.debug("Sending XNAP PDU. bytes:{}", tx_pdu.length());
  xnc_gw.init_association(xnap_cfg.peer_addr, std::move(tx_pdu));
  logger.debug("XNAP PDU sent");
}
