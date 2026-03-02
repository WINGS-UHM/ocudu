// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#include "xn_setup_procedure.h"
#include "../xnap_asn1_utils.h"
#include "xn_setup_procedure_asn1_helpers.h"
#include "ocudu/support/async/async_timer.h"

using namespace ocudu;
using namespace ocudu::ocucp;
using namespace asn1::xnap;

static constexpr std::chrono::milliseconds xn_setup_response_timeout{5000};

xn_setup_procedure::xn_setup_procedure(const xnap_configuration&          xnap_cfg_,
                                       xnap_tx_pdu_notifier_with_logging& tx_notifier_,
                                       xnap_transaction_manager&          ev_mng_,
                                       timer_factory                      timers_,
                                       ocudulog::basic_logger&            logger_) :
  xnap_cfg(xnap_cfg_),
  tx_notifier(tx_notifier_),
  ev_mng(ev_mng_),
  logger(logger_),
  xn_setup_wait_timer(timers_.create_timer())
{
}

void xn_setup_procedure::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.info("\"{}\" started...", name());

  // Prepare XN Setup message.
  xn_setup_req = generate_asn1_xn_setup_request(xnap_cfg);

  while (true) {
    // Subscribe to respective publisher to receive XN SETUP RESPONSE/FAILURE message.
    transaction_sink.subscribe_to(ev_mng.xn_setup_outcome, xn_setup_response_timeout);

    // Forward message to XN-C.
    if (!tx_notifier.on_xn_setup_request(xn_setup_req)) {
      logger.warning("Cannot send XNSetupRequest");
      // TODO: Return XN setup failure
      CORO_EARLY_RETURN();
    }

    // Await XN Setup response.
    CORO_AWAIT(transaction_sink);

    if (not retry_required()) {
      // No more attempts. Exit loop.
      break;
    }

    // Await timer.
    logger.debug("Reinitiating XN setup in {}s. Received XNSetupFailure with Time to Wait IE", time_to_wait.count());
    CORO_AWAIT(
        async_wait_for(xn_setup_wait_timer, std::chrono::duration_cast<std::chrono::milliseconds>(time_to_wait)));
  }

  logger.info("\"{}\" finished successfully", name());

  // TODO: Return XN setup response.
  CORO_RETURN();
}

bool xn_setup_procedure::retry_required()
{
  if (transaction_sink.successful()) {
    // Success case.
    return false;
  }

  if (transaction_sink.timeout_expired()) {
    // Timeout case.
    logger.warning("\"{}\" timed out after {}ms", name(), xn_setup_response_timeout.count());
    fmt::print("\"{}\" timed out after {}ms", name(), xn_setup_response_timeout.count());
    return false;
  }

  if (!transaction_sink.failed()) {
    // No response received.
    logger.warning("\"{}\" failed. No response received", name());
    fmt::print("\"{}\" failed. No response received\n", name());
    return false;
  }

  const asn1::xnap::xn_setup_fail_s& xn_fail = transaction_sink.failure();

  if (not xn_fail->time_to_wait_present) {
    // XN-C peer didn't command a waiting time.
    logger.warning("\"{}\": Stopping procedure. Cause: XN-C peer did not set any retry waiting time", name());
    logger.warning("\"{}\" failed. XN-C peer Cause: \"{}\"", name(), asn1_utils::get_cause_str(xn_fail->cause));
    fmt::print("\"{}\" failed. XN-C peer Cause: \"{}\"\n", name(), asn1_utils::get_cause_str(xn_fail->cause));
    return false;
  }

  time_to_wait = std::chrono::seconds{xn_fail->time_to_wait.to_number()};
  return true;
}
