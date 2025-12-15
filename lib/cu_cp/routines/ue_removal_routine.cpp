/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ue_removal_routine.h"

using namespace ocudu;
using namespace ocudu::ocucp;

ue_removal_routine::ue_removal_routine(ue_index_t                           ue_index_,
                                       rrc_ue_handler*                      rrc_du_notifier_,
                                       e1ap_bearer_context_removal_handler* e1ap_removal_handler_,
                                       f1ap_ue_context_removal_handler*     f1ap_removal_handler_,
                                       ngap_ue_context_removal_handler*     ngap_removal_handler_,
                                       nrppa_ue_context_removal_handler*    nrppa_removal_handler_,
                                       ue_manager&                          ue_mng_,
                                       ocudulog::basic_logger&              logger_) :
  ue_index(ue_index_),
  rrc_du_notifier(rrc_du_notifier_),
  e1ap_removal_handler(e1ap_removal_handler_),
  f1ap_removal_handler(f1ap_removal_handler_),
  ngap_removal_handler(ngap_removal_handler_),
  nrppa_removal_handler(nrppa_removal_handler_),
  ue_mng(ue_mng_),
  logger(logger_)
{
}

void ue_removal_routine::operator()(coro_context<async_task<void>>& ctx)
{
  CORO_BEGIN(ctx);

  logger.debug("ue={}: \"{}\" started...", ue_index, name());

  // Remove RRC UE if it exists.
  if (rrc_du_notifier != nullptr) {
    rrc_du_notifier->remove_ue(ue_index);
  }
  // Remove Bearer Context from E1AP if it exists.
  if (e1ap_removal_handler != nullptr) {
    e1ap_removal_handler->remove_bearer_context(ue_index);
  }

  // Remove UE Context from F1AP if it exists.
  if (f1ap_removal_handler != nullptr) {
    f1ap_removal_handler->remove_ue_context(ue_index);
  }
  // Remove UE Context from NGAP if it exists.
  if (ngap_removal_handler != nullptr) {
    ngap_removal_handler->remove_ue_context(ue_index);
  }

  // Remove UE Context from NRPPa if it exists.
  if (nrppa_removal_handler != nullptr) {
    nrppa_removal_handler->remove_ue_context(ue_index);
  }

  // Remove UE from UE manager.
  ue_mng.remove_ue(ue_index);

  logger.info("ue={}: \"{}\" finished successfully", ue_index, name());

  CORO_RETURN();
}
