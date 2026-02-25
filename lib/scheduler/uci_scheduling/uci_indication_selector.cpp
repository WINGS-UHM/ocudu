/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "uci_indication_selector.h"
#include "ocudu/ocudulog/ocudulog.h"

using namespace ocudu;

uci_indication_selector::uci_indication_selector(uci_indication_timeout_notifier& timeout_notifier_,
                                                 unsigned                         uci_ack_timeout,
                                                 unsigned                         max_pucch_grants_per_slot) :
  ack_timeout_slots(uci_ack_timeout),
  timeout_notifier(timeout_notifier_),
  logger(ocudulog::fetch_basic_logger("SCHED")),
  uci_wheel(ack_timeout_slots, invalid_entry_id),
  short_timeout_wheel(SHORT_PUCCH_TIMEOUT_SLOTS, invalid_entry_id)
{
  uci_pool.reserve(
      std::min<unsigned>(ack_timeout_slots * std::min<unsigned>(max_pucch_grants_per_slot, MAX_PUCCH_PDUS_PER_SLOT),
                         MAX_NOF_DU_UES * MAX_NOF_HARQS));
  report_fatal_error_if_not(uci_ack_timeout > SHORT_PUCCH_TIMEOUT_SLOTS, "Invalid UCI ACK timeout");
}

/// Convert UCI indication to an action.
static uci_action create_action(const uci_indication::uci_pdu& pdu)
{
  uci_action ret;
  bool       is_dtx = false;
  if (const auto* f0f1 = std::get_if<uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu>(&pdu.pdu)) {
    ret.type                = uci_action::pdu_type::pucch_f0f1;
    ret.ul_sinr_dB          = f0f1->ul_sinr_dB;
    ret.time_advance_offset = f0f1->time_advance_offset;
    ret.sr_detected         = f0f1->sr_detected;
    ret.harq_ack_bits.resize(f0f1->harqs.size());
    for (unsigned i = 0, e = f0f1->harqs.size(); i != e; ++i) {
      if (f0f1->harqs[i] == mac_harq_ack_report_status::ack) {
        ret.harq_ack_bits.set(i);
      }
      is_dtx |= f0f1->harqs[i] == mac_harq_ack_report_status::dtx;
    }
  } else if (const auto* f2f3f4 = std::get_if<uci_indication::uci_pdu::uci_pucch_f2_or_f3_or_f4_pdu>(&pdu.pdu)) {
    ret.type                = uci_action::pdu_type::pucch_f2f3f4;
    ret.ul_sinr_dB          = f2f3f4->ul_sinr_dB;
    ret.time_advance_offset = f2f3f4->time_advance_offset;
    ret.sr_detected         = f2f3f4->sr_info.any();
    ret.harq_ack_bits.resize(f2f3f4->harqs.size());
    for (unsigned i = 0, e = f2f3f4->harqs.size(); i != e; ++i) {
      if (f2f3f4->harqs[i] == mac_harq_ack_report_status::ack) {
        ret.harq_ack_bits.set(i);
      }
      is_dtx |= f2f3f4->harqs[i] == mac_harq_ack_report_status::dtx;
    }
    ret.csi = f2f3f4->csi;
  } else {
    const auto& pusch = std::get<uci_indication::uci_pdu::uci_pusch_pdu>(pdu.pdu);
    ret.type          = uci_action::pdu_type::pusch;
    ret.harq_ack_bits.resize(pusch.harqs.size());
    for (unsigned i = 0, e = pusch.harqs.size(); i != e; ++i) {
      if (pusch.harqs[i] == mac_harq_ack_report_status::ack) {
        ret.harq_ack_bits.set(i);
      }
      is_dtx |= pusch.harqs[i] == mac_harq_ack_report_status::dtx;
    }
    ret.csi = pusch.csi;
  }
  ret.uci_valid =
      (ret.sr_detected or (not ret.harq_ack_bits.empty() and not is_dtx) or (ret.csi.has_value() and ret.csi->valid));
  return ret;
}

static bool has_harq_ack_bits(const uci_indication::uci_pdu& pdu)
{
  if (auto* f0f1 = std::get_if<uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu>(&pdu.pdu)) {
    return not f0f1->harqs.empty();
  }
  if (auto* f2f3f4 = std::get_if<uci_indication::uci_pdu::uci_pucch_f2_or_f3_or_f4_pdu>(&pdu.pdu)) {
    return not f2f3f4->harqs.empty();
  }
  return not std::get<uci_indication::uci_pdu::uci_pusch_pdu>(pdu.pdu).harqs.empty();
}

std::optional<uci_action> uci_indication_selector::handle_uci_ind_pdu(slot_point                     sl_rx,
                                                                      const uci_indication::uci_pdu& pdu)
{
  // If the PDU has no HARQ-ACK bits, it was not registered for timeout tracking (e.g. SR-only, CSI-only or SR-CSI-only
  // PUCCH). In this case, create and return an action directly.
  if (not has_harq_ack_bits(pdu)) {
    return create_action(pdu);
  }

  stable_id_t& list_head  = uci_wheel[sl_rx.count()];
  uci_entry*   prev_entry = nullptr;
  for (stable_id_t id = list_head; id != invalid_entry_id;) {
    uci_entry& entry = uci_pool[id];

    if (pdu.crnti == entry.crnti) {
      // RNTIs match. The grant was found.

      // Generate an action.
      auto action = handle_uci_pdu(pdu, entry);

      if (action.has_value()) {
        // An action was generated. It means that all the combining is complete and we can erase the UCI entry.
        rem_uci_entry(list_head, prev_entry, entry);
      } else {
        // No action was generated. This means that the UCI entry is still expecting more UCI indications.
        if (not entry.short_timeout_slot.valid()) {
          // We add the UCI grant in another linked list in the short timeout wheel, if not added yet.
          entry.short_timeout_slot = last_sl_tx + SHORT_PUCCH_TIMEOUT_SLOTS;
          auto& short_timeout_head = short_timeout_wheel[entry.short_timeout_slot.count()];
          entry.next_short_timeout = short_timeout_head;
          short_timeout_head       = id;
        }
      }

      return action;
    }

    prev_entry = &entry;
    id         = entry.next;
  }

  logger.warning("rnti={}: Discarding UCI indication PDU. Cause: Respective UCI grant was not found (UCI slot={})",
                 pdu.crnti,
                 sl_rx);

  return std::nullopt;
}

std::optional<uci_action> uci_indication_selector::handle_uci_pdu(const uci_indication::uci_pdu& pdu, uci_entry& entry)
{
  // Retrieve info from different PUCCH/PUSCH formats.
  uci_action ret = create_action(pdu);

  // Case: If there was no previous UCI PDU decoded, it had lower SNR or was invalid, this UCI PDU is chosen.
  if (ret.uci_valid and
      (not entry.chosen_action.ul_sinr_dB.has_value() or
       (ret.ul_sinr_dB.has_value() and entry.chosen_action.ul_sinr_dB.value() < ret.ul_sinr_dB.value()))) {
    entry.chosen_action = ret;
  }

  if (entry.uci_pdus_to_rx <= 1) {
    // Case: This is the last PUCCH that is expected for this UCI grant.
    entry.uci_pdus_to_rx = 0;
    return entry.chosen_action;
  }

  // Case: This is not the last PUCCH that is expected for this UCI grant.
  entry.uci_pdus_to_rx--;

  return std::nullopt;
}

void uci_indication_selector::handle_timeouts(slot_point sl_tx)
{
  // Handle UCI entries that reach their timeout and never received any UCI PDU.
  slot_point sl_rx = sl_tx - ack_timeout_slots;
  for (auto& uci_ids_to_rem = uci_wheel[sl_rx.count()]; uci_ids_to_rem != invalid_entry_id;) {
    uci_entry& entry = uci_pool[uci_ids_to_rem];
    ocudu_sanity_check(entry.uci_slot == sl_rx, "Invalid wheel state");

    logger.warning("rnti={}: Forcing \"NACK\" for {} DL HARQ processes. Cause: Timeout was reached ({} slots) "
                   "but no UCI indication feedback has been received yet from lower layers (UCI slot={})",
                   entry.crnti,
                   entry.chosen_action.harq_ack_bits.size(),
                   ack_timeout_slots,
                   entry.uci_slot);

    // Handle UCI timeout if there were still pending UCI indications.
    timeout_notifier.on_timeout(sl_rx, entry.crnti, entry.chosen_action);

    // Remove UCI grant entry from linked list and pool.
    rem_uci_entry(uci_ids_to_rem, nullptr, entry);
  }
  ocudu_sanity_check(uci_wheel[sl_rx.count()] == invalid_entry_id, "Unexpected state for UCI time wheel");

  // Handle UCI entries that received at least one but not all the expected UCI indications (within the short timeout
  // window).
  for (auto& uci_ids_to_rem = short_timeout_wheel[sl_tx.count()]; uci_ids_to_rem != invalid_entry_id;) {
    uci_entry& entry = uci_pool[uci_ids_to_rem];

    // Handle UCI timeout.
    if (entry.chosen_action.uci_valid) {
      logger.debug("rnti={}: Forwarding HARQ-ACK bits=0b{:b} to UE DL HARQ processes without all UCI indication "
                   "feedback having been received. Cause: Timeout was reached ({} slots), but at least a valid UCI "
                   "PDU was received (UCI slot={}).",
                   entry.crnti,
                   entry.chosen_action.harq_ack_bits,
                   SHORT_PUCCH_TIMEOUT_SLOTS,
                   entry.uci_slot);
    } else {
      // At least one of the expected ACKs went missing and we haven't received any valid UCI.
      logger.warning("rnti={}: Forcing \"NACK\" for {} DL HARQ processes. Cause: Timeout was reached ({} slots) "
                     "to receive the respective UCI indication feedback and no valid UCI PDU has been received yet "
                     "(UCI slot={})",
                     entry.crnti,
                     entry.chosen_action.harq_ack_bits.size(),
                     SHORT_PUCCH_TIMEOUT_SLOTS,
                     entry.uci_slot);
    }

    // Propagate timeout.
    timeout_notifier.on_timeout(entry.uci_slot, entry.crnti, entry.chosen_action);

    // Delete entry from both wheels and pool.
    auto id_to_rem = uci_ids_to_rem;
    uci_ids_to_rem = entry.next_short_timeout;
    find_and_rem_uci_entry<true>(id_to_rem);
    uci_pool.erase(id_to_rem);
  }
  ocudu_sanity_check(short_timeout_wheel[sl_tx.count()] == invalid_entry_id,
                     "Unexpected state for short UCI timeout wheel");
}

void uci_indication_selector::handle_result(slot_point sl_tx, const sched_result& result)
{
  // Handle UCI grant timeouts accounting for potential slot indication skips.
  unsigned skipped_slots = 1;
  if (OCUDU_LIKELY(last_sl_tx.valid())) {
    skipped_slots = std::min<unsigned>(sl_tx - last_sl_tx, uci_wheel.size());
  }
  for (unsigned i = 0; i != skipped_slots; ++i) {
    handle_timeouts(sl_tx + 1 - skipped_slots + i);
  }
  last_sl_tx = sl_tx;

  // Handle new PUCCH grants scheduled in this slot.
  auto& uci_wheel_sl_tx = uci_wheel[sl_tx.count()];
  ocudu_sanity_check(uci_wheel_sl_tx == invalid_entry_id, "The wheel should be empty for slot tx");
  for (const pucch_info& pucch : result.ul.pucchs) {
    if (pucch.uci_bits.harq_ack_nof_bits == 0) {
      // Only PUCCHs with HARQ-ACK bits need to be buffered for timeout handling.
      continue;
    }

    // Check if there is another PUCCH (e.g. F1 SR + F1 HARQ-ACK case or during transition from fallback).
    // If so, increment uci_pdus_to_rx.
    {
      auto id = uci_wheel_sl_tx;
      while (id != invalid_entry_id) {
        auto& entry = uci_pool[id];
        if (entry.crnti == pucch.crnti) {
          entry.uci_pdus_to_rx++;
          break;
        }
        id = entry.next;
      }
      if (id != invalid_entry_id) {
        // Another PUCCH F1 was found in this slot. Avoid adding more than one UCI grant for the same RNTI in the wheel.
        continue;
      }
    }

    // Create new UCI entry and save it in the UCI wheel.
    uci_entry entry;
    entry.crnti          = pucch.crnti;
    entry.uci_pdus_to_rx = 1;
    entry.uci_slot       = sl_tx;
    // The chosen action set here is what will be propagated in case of timeout.
    entry.chosen_action.harq_ack_bits.resize(pucch.uci_bits.harq_ack_nof_bits);
    entry.next      = uci_wheel_sl_tx;
    stable_id_t id  = uci_pool.insert(entry);
    uci_wheel_sl_tx = id;
  }

  // Handle new PUSCH with UCI grants scheduled in this slot.
  for (const ul_sched_info& pusch : result.ul.puschs) {
    if (not pusch.uci.has_value() or not pusch.uci->harq.has_value() or pusch.uci->harq->harq_ack_nof_bits == 0) {
      // Only UCI with HARQ-ACK bits need to be buffered for timeout handling.
      continue;
    }

    // Create new UCI entry and save it in the UCI wheel.
    uci_entry entry;
    entry.crnti          = pusch.pusch_cfg.rnti;
    entry.uci_pdus_to_rx = 1;
    entry.uci_slot       = sl_tx;
    entry.chosen_action.harq_ack_bits.resize(pusch.uci->harq->harq_ack_nof_bits);
    entry.next      = uci_wheel_sl_tx;
    stable_id_t id  = uci_pool.insert(entry);
    uci_wheel_sl_tx = id;
  }
}

void uci_indication_selector::handle_discarded_ucis(slot_point sl_tx)
{
  for (auto& id_to_rem = uci_wheel[sl_tx.count()]; id_to_rem != invalid_entry_id;) {
    uci_entry& entry = uci_pool[id_to_rem];

    // The lower layers will not attempt to decode the PUCCHs and PUSCH UCIs and will not send any UCI indication
    // feedback. To avoid a long DL HARQ timeout window (due to lack of UCI indication), it is important to force a DTX
    // for the DL HARQ processes with UCI falling in this slot.
    // Note: We don't use this cancellation to update the DL OLLA (UCI is invalid), as we shouldn't take lates into
    // account in link adaptation.
    timeout_notifier.on_timeout(sl_tx, entry.crnti, entry.chosen_action);

    // Erase UCI entry.
    rem_uci_entry(id_to_rem, nullptr, entry);
  }
}

stable_id_t uci_indication_selector::rem_uci_entry(stable_id_t& head, uci_entry* prev_entry, uci_entry& entry)
{
  stable_id_t       entry_rid  = invalid_entry_id;
  const stable_id_t entry_next = entry.next;
  if (prev_entry == nullptr) {
    // Entry is the head of the linked list.
    entry_rid = head;
    head      = entry.next;
  } else {
    // Entry is not the head of the linked list.
    entry_rid        = prev_entry->next;
    prev_entry->next = entry.next;
  }
  entry.next = invalid_entry_id;

  if (entry.short_timeout_slot.valid()) {
    // Entry is also in the short combining timeout list.
    // Do O(N) removal; however, list should be very short.
    find_and_rem_uci_entry<false>(entry_rid);
  }

  uci_pool.erase(entry_rid);
  return entry_next;
}

template <bool MainTimeoutWheel>
void uci_indication_selector::find_and_rem_uci_entry(stable_id_t id_to_rem)
{
  auto&        rem_entry   = uci_pool[id_to_rem];
  stable_id_t& uci_id_head = MainTimeoutWheel ? uci_wheel[rem_entry.uci_slot.count()]
                                              : short_timeout_wheel[rem_entry.short_timeout_slot.count()];
  auto         get_next    = [](uci_entry& entry) -> stable_id_t& {
    return MainTimeoutWheel ? entry.next : entry.next_short_timeout;
  };

  if (uci_id_head == id_to_rem) {
    // The element to remove is at the head of the list.
    uci_id_head = get_next(rem_entry);
  } else {
    for (stable_id_t id = uci_id_head; id != invalid_entry_id;) {
      auto& prev_entry = uci_pool[id];

      if (get_next(prev_entry) == id_to_rem) {
        // Next is the entry to be removed.
        get_next(prev_entry) = get_next(rem_entry);
        break;
      }

      id = get_next(prev_entry);
    }
  }

  // Clear next pointers.
  get_next(rem_entry) = invalid_entry_id;
  if (MainTimeoutWheel) {
    rem_entry.uci_slot = {};
  } else {
    rem_entry.short_timeout_slot = {};
  }
}
