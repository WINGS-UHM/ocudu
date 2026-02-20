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

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

xn_setup_procedure::xn_setup_procedure(const xnap_configuration&          xnap_cfg_,
                                       xnap_tx_pdu_notifier_with_logging& tx_notifier_,
                                       ocudulog::basic_logger&            logger_) :
  xnap_cfg(xnap_cfg_), tx_notifier(tx_notifier_), logger(logger_)
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
  if (!tx_notifier.on_new_message(xn_setup_req)) {
    logger.error("Failed to send XN Setup Request");
  } else {
    logger.debug("XNAP PDU sent");
  }
}
