/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "message_bufferer_slot_gateway_impl.h"
#include "ocudu/ocudulog/ocudulog.h"

using namespace ocudu;
using namespace fapi;

message_bufferer_slot_gateway_impl::message_bufferer_slot_gateway_impl(unsigned             sector_id_,
                                                                       unsigned             l2_nof_slots_ahead_,
                                                                       subcarrier_spacing   scs_,
                                                                       p7_requests_gateway& p7_gateway_) :
  sector_id(sector_id_),
  l2_nof_slots_ahead(l2_nof_slots_ahead_),
  scs(scs_),
  p7_gateway(p7_gateway_),
  logger(ocudulog::fetch_basic_logger("FAPI"))
{
  // Pool size is 1 unit bigger than the number of slots the L2 is ahead of the L1, in case that an incoming message
  // arrives before the notification of a new slot.
  unsigned pool_size = l2_nof_slots_ahead + 1;

  ocudu_assert(pool_size <= MAX_NUM_BUFFERED_MESSAGES,
               "The requested pool size '{}' is bigger than the supported '{}'",
               pool_size,
               MAX_NUM_BUFFERED_MESSAGES);

  dl_tti_pool.resize(pool_size);
  ul_tti_pool.resize(pool_size);
  ul_dci_pool.resize(pool_size);
  tx_data_pool.resize(pool_size);
}

template <typename T, typename P, typename Function>
void message_bufferer_slot_gateway_impl::handle_message(T&& msg, P pool, Function func)
{
  slot_point msg_slot     = msg.slot;
  slot_point current_slot = get_current_slot();

  int difference = msg_slot - current_slot;

  if (difference > int(l2_nof_slots_ahead)) {
    logger.warning("Sector#{}: Skipping FAPI message as the difference '{}' between the slot of the message and the "
                   "current slot '{}' "
                   "is bigger than the configured delay '{}'",
                   sector_id,
                   difference,
                   current_slot,
                   l2_nof_slots_ahead);

    return;
  }

  // Same slot or older, no need to store the message.
  if (difference <= 0) {
    func(std::forward<T>(msg));

    return;
  }

  auto& msg_pool_entry = pool[msg_slot.system_slot() % pool.size()];

  if (msg_pool_entry) {
    logger.warning("Sector#{}: Detected unsent cached FAPI message type '{}' for slot '{}'",
                   sector_id,
                   fmt::underlying(msg_pool_entry->message_type),
                   msg_pool_entry->slot);
  }

  msg_pool_entry.emplace(std::forward<T>(msg));
}

/// Sends the message cached in the given pool for the given slot using the given function and clears the pool
/// entry.
template <typename T, typename Function>
static void send_message(slot_point              slot,
                         subcarrier_spacing      scs,
                         unsigned                sector_id,
                         ocudulog::basic_logger& logger,
                         span<std::optional<T>>  pool,
                         Function                func)
{
  std::optional<T>& pool_entry = pool[slot.system_slot() % pool.size()];
  if (!pool_entry) {
    return;
  }

  if (auto msg_type = pool_entry->message_type; pool_entry->slot == slot) {
    func(*pool_entry);
  } else {
    logger.warning("Sector#{}: Detected unsent cached FAPI message id '{}' for slot '{}'",
                   sector_id,
                   fmt::underlying(msg_type),
                   pool_entry->slot);
  }

  pool_entry.reset();
}

void message_bufferer_slot_gateway_impl::forward_cached_messages(slot_point slot)
{
  send_message<dl_tti_request>(slot, scs, sector_id, logger, dl_tti_pool, [this](const dl_tti_request& msg) {
    p7_gateway.send_dl_tti_request(msg);
  });

  send_message<ul_tti_request>(slot, scs, sector_id, logger, ul_tti_pool, [this](const ul_tti_request& msg) {
    p7_gateway.send_ul_tti_request(msg);
  });

  send_message<ul_dci_request>(slot, scs, sector_id, logger, ul_dci_pool, [this](const ul_dci_request& msg) {
    p7_gateway.send_ul_dci_request(msg);
  });

  send_message<tx_data_request>(slot, scs, sector_id, logger, tx_data_pool, [this](const tx_data_request& msg) {
    p7_gateway.send_tx_data_request(msg);
  });
}

void message_bufferer_slot_gateway_impl::handle_dl_tti_request(const dl_tti_request& msg)
{
  handle_message(msg, span<std::optional<dl_tti_request>>(dl_tti_pool), [this](const dl_tti_request& message) {
    p7_gateway.send_dl_tti_request(message);
  });
}

void message_bufferer_slot_gateway_impl::handle_ul_tti_request(const ul_tti_request& msg)
{
  handle_message(msg, span<std::optional<ul_tti_request>>(ul_tti_pool), [this](const ul_tti_request& message) {
    p7_gateway.send_ul_tti_request(message);
  });
}

void message_bufferer_slot_gateway_impl::handle_ul_dci_request(const ul_dci_request& msg)
{
  handle_message(msg, span<std::optional<ul_dci_request>>(ul_dci_pool), [this](const ul_dci_request& message) {
    p7_gateway.send_ul_dci_request(message);
  });
}

void message_bufferer_slot_gateway_impl::handle_tx_data_request(const tx_data_request& msg)
{
  handle_message(msg, span<std::optional<tx_data_request>>(tx_data_pool), [this](const tx_data_request& message) {
    p7_gateway.send_tx_data_request(message);
  });
}
