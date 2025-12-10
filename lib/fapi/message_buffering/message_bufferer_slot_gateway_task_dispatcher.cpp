/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "message_bufferer_slot_gateway_task_dispatcher.h"
#include "ocudu/fapi/p7/messages/dl_tti_request.h"
#include "ocudu/fapi/p7/messages/tx_data_request.h"
#include "ocudu/fapi/p7/messages/ul_dci_request.h"
#include "ocudu/fapi/p7/messages/ul_tti_request.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/support/executors/task_executor.h"
#include "ocudu/support/rtsan.h"

using namespace ocudu;
using namespace fapi;

message_bufferer_slot_gateway_task_dispatcher::message_bufferer_slot_gateway_task_dispatcher(
    unsigned                   sector_id_,
    unsigned                   l2_nof_slots_ahead,
    subcarrier_spacing         scs_,
    fapi::p7_requests_gateway& p7_gateway,
    task_executor&             executor_) :
  sector_id(sector_id_),
  logger(ocudulog::fetch_basic_logger("FAPI")),
  executor(executor_),
  message_bufferer_gateway(sector_id, l2_nof_slots_ahead, scs_, p7_gateway)
{
}

void message_bufferer_slot_gateway_task_dispatcher::send_dl_tti_request(const dl_tti_request& msg)
{
  if (!executor.defer(
          [this, msg]() noexcept OCUDU_RTSAN_NONBLOCKING { message_bufferer_gateway.handle_dl_tti_request(msg); })) {
    logger.warning("Sector#{}: Failed to cache DL_TTI.request message for slot '{}'", sector_id, msg.slot);
  }
}

void message_bufferer_slot_gateway_task_dispatcher::send_ul_tti_request(const ul_tti_request& msg)
{
  if (!executor.defer(
          [this, msg]() noexcept OCUDU_RTSAN_NONBLOCKING { message_bufferer_gateway.handle_ul_tti_request(msg); })) {
    logger.warning("Sector#{}: Failed to cache UL_TTI.request message for slot '{}'", sector_id, msg.slot);
  }
}

void message_bufferer_slot_gateway_task_dispatcher::send_ul_dci_request(const ul_dci_request& msg)
{
  if (!executor.defer(
          [this, msg]() noexcept OCUDU_RTSAN_NONBLOCKING { message_bufferer_gateway.handle_ul_dci_request(msg); })) {
    logger.warning("Sector#{}: Failed to cache UL_DCI.request message for slot '{}'", sector_id, msg.slot);
  }
}

void message_bufferer_slot_gateway_task_dispatcher::send_tx_data_request(const tx_data_request& msg)
{
  if (!executor.defer(
          [this, msg]() noexcept OCUDU_RTSAN_NONBLOCKING { message_bufferer_gateway.handle_tx_data_request(msg); })) {
    logger.warning("Sector#{}: Failed to cache TX_Data.request message for slot '{}'", sector_id, msg.slot);
  }
}

void message_bufferer_slot_gateway_task_dispatcher::update_current_slot(slot_point slot)
{
  message_bufferer_gateway.update_current_slot(slot);
}

void message_bufferer_slot_gateway_task_dispatcher::forward_cached_messages(slot_point slot)
{
  if (!executor.defer([this, slot]() noexcept OCUDU_RTSAN_NONBLOCKING {
        message_bufferer_gateway.forward_cached_messages(slot);
      })) {
    logger.warning("Sector#{}: Failed to dispatch cached messages for slot '{}'", sector_id, slot);
  }
}
