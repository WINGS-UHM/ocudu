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

uci_indication_selector::uci_indication_selector(unsigned                         uci_ack_timeout,
                                                 unsigned                         max_pucch_grants_per_slot,
                                                 uci_indication_timeout_notifier& timeout_notifier_) :
  ack_timeout_slots(uci_ack_timeout),
  timeout_notifier(timeout_notifier_),
  logger(ocudulog::fetch_basic_logger("SCHED")),
  uci_wheel(ack_timeout_slots, invalid_row_id),
  short_timeout_wheel(SHORT_PUCCH_TIMEOUT_SLOTS, invalid_row_id)
{
  uci_pool.reserve(
      std::min<unsigned>(ack_timeout_slots * std::min<unsigned>(max_pucch_grants_per_slot, MAX_PUCCH_PDUS_PER_SLOT),
                         MAX_NOF_DU_UES * MAX_NOF_HARQS));
  report_fatal_error_if_not(uci_ack_timeout > SHORT_PUCCH_TIMEOUT_SLOTS, "Invalid UCI ACK timeout");
}

std::optional<uci_action> uci_indication_selector::handle_uci_ind_pdu(slot_point                     sl_rx,
                                                                      const uci_indication::uci_pdu& pdu)
{
  soa::row_id& list_head = uci_wheel[sl_rx.count()];

  for (soa::row_id prev_rid = invalid_row_id, rid = list_head; rid != invalid_row_id;) {
    uci_entry& entry = uci_pool.at<slot_column_id::uci>(rid);

    if (pdu.crnti == entry.crnti) {
      // RNTIs match. The grant was found.

      // Generate an action.
      auto action = handle_uci_pdu(pdu, entry);

      if (action.has_value()) {
        // An action was generated. Before returning, remove entry from wheel linked lists and pool.
        if (entry.short_timeout_wheel_pos.valid()) {
          // The entry is in the short timeout wheel. Remove it.
          auto& short_timeout = short_timeout_wheel[entry.short_timeout_wheel_pos.count()];
          for (soa::row_id prev_rem_rid = invalid_row_id, rem_rid = short_timeout; rem_rid != invalid_row_id;) {
            auto& rem_entry = uci_pool.at<slot_column_id::uci>(rem_rid);
            if (&rem_entry == &entry) {
              // Found matching entry in short timeout wheel. Remove it from the linked list.
              if (prev_rem_rid != invalid_row_id) {
                auto& prev_rem_entry = uci_pool.at<slot_column_id::uci>(prev_rem_rid);
                prev_rem_entry.next  = rem_entry.next;
              } else {
                short_timeout = rem_entry.next;
              }
              break;
            }
            prev_rem_rid = rem_rid;
            rem_rid      = rem_entry.next;
          }
        }

        // Remote entry from main UCI wheel.
        if (prev_rid != invalid_row_id) {
          // Case where entry is not head of list.
          uci_entry& prev_entry = uci_pool.at<slot_column_id::uci>(prev_rid);
          prev_entry.next       = entry.next;
        } else {
          // It is head of the list.
          list_head = entry.next;
        }

        // Delete UCI entry.
        uci_pool.erase(rid);
      } else {
        // No action was generated. This means that the UCI is still expecting another PUCCH.
        if (not entry.short_timeout_wheel_pos.valid()) {
          // We add the UCI grant in another linked list in the short timeout wheel, if not added yet.
          entry.short_timeout_wheel_pos = last_sl_tx + SHORT_PUCCH_TIMEOUT_SLOTS;
          auto& short_timeout_head      = short_timeout_wheel[entry.short_timeout_wheel_pos.count()];
          entry.next_short_timeout      = short_timeout_head;
          short_timeout_head            = rid;
        }
      }

      return action;
    }

    prev_rid = rid;
    rid      = entry.next;
  }

  logger.warning("rnti={}: UCI not found for slot={}", pdu.crnti, sl_rx);

  return std::nullopt;
}

std::optional<uci_action> uci_indication_selector::handle_uci_pdu(const uci_indication::uci_pdu& pdu, uci_entry& entry)
{
  // Retrieve info from different PUCCH/PUSCH formats.
  uci_action           ret;
  bool                 is_dtx = false;
  std::optional<float> snr;
  if (const auto* f0f1 = std::get_if<uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu>(&pdu.pdu)) {
    snr             = f0f1->ul_sinr_dB;
    ret.sr_detected = f0f1->sr_detected;
    ret.harq_ack_bits.resize(f0f1->harqs.size());
    for (unsigned i = 0, e = f0f1->harqs.size(); i != e; ++i) {
      if (f0f1->harqs[i] == mac_harq_ack_report_status::ack) {
        ret.harq_ack_bits.set(i);
      }
      is_dtx |= f0f1->harqs[i] == mac_harq_ack_report_status::dtx;
    }
  } else if (const auto* f2f3f4 = std::get_if<uci_indication::uci_pdu::uci_pucch_f2_or_f3_or_f4_pdu>(&pdu.pdu)) {
    snr             = f2f3f4->ul_sinr_dB;
    ret.sr_detected = f2f3f4->sr_info.any();
    ret.harq_ack_bits.resize(f2f3f4->harqs.size());
    for (unsigned i = 0, e = f2f3f4->harqs.size(); i != e; ++i) {
      if (f2f3f4->harqs[i] == mac_harq_ack_report_status::ack) {
        ret.harq_ack_bits.set(i);
      }
      is_dtx |= f2f3f4->harqs[i] == mac_harq_ack_report_status::dtx;
    }
  } else {
    const auto& pusch = std::get<uci_indication::uci_pdu::uci_pusch_pdu>(pdu.pdu);
    ret.harq_ack_bits.resize(pusch.harqs.size());
    for (unsigned i = 0, e = pusch.harqs.size(); i != e; ++i) {
      if (pusch.harqs[i] == mac_harq_ack_report_status::ack) {
        ret.harq_ack_bits.set(i);
      }
      is_dtx |= pusch.harqs[i] == mac_harq_ack_report_status::dtx;
    }
  }

  if (not std::holds_alternative<uci_indication::uci_pdu::uci_pucch_f0_or_f1_pdu>(pdu.pdu)) {
    // It is not PUCCH F1. We do not need to decide which is the best PUCCH candidate.
    ocudu_sanity_check(entry.pucchs_to_rx <= 1, "Invalid pucch_to_rx value");
    entry.pucchs_to_rx = 0;
    return ret;
  }

  if (not is_dtx and (not entry.last_snr.has_value() or (snr.has_value() and entry.last_snr.value() < snr.value()))) {
    // Case: If there was no previous HARQ-ACK decoded or the previous HARQ-ACK had lower SNR, this HARQ-ACK is chosen.
    entry.chosen_action = ret;
    entry.last_snr      = snr;
  }

  if (entry.pucchs_to_rx <= 1) {
    // Case: This is the last PUCCH that is expected for this UCI grant.
    entry.pucchs_to_rx = 0;
    return entry.chosen_action;
  }

  // Case: This is not the last PUCCH that is expected for this UCI grant.
  entry.pucchs_to_rx--;

  return std::nullopt;
}

void uci_indication_selector::handle_result(slot_point sl_tx, const sched_result& result)
{
  last_sl_tx = sl_tx;

  // Handle UCI timeouts.
  slot_point sl_rx             = sl_tx - ack_timeout_slots;
  auto&      uci_wheel_timeout = uci_wheel[sl_rx.count()];
  for (soa::row_id rid = uci_wheel_timeout; rid != invalid_row_id;) {
    uci_entry& entry = uci_pool.at<slot_column_id::uci>(rid);

    // Handle UCI timeout.
    if (entry.pucchs_to_rx > 0) {
      timeout_notifier.on_timeout(sl_rx, entry.crnti, entry.chosen_action);
    }

    // Remove UCI grant entry from linked list and pool.
    soa::row_id next = entry.next;
    ocudu_sanity_check(entry.next_short_timeout == invalid_row_id,
                       "Trying to erase entry that is still present in the short timeout wheel");
    uci_pool.erase(rid);

    rid = next;
  }
  uci_wheel_timeout = invalid_row_id;

  // Handle UCIs that received at least one but not all the expected PUCCHs.
  auto& rem_pucch_timeout = short_timeout_wheel[sl_tx.count()];
  for (soa::row_id rid = rem_pucch_timeout; rid != invalid_row_id;) {
    uci_entry& entry = uci_pool.at<slot_column_id::uci>(rid);

    // Handle UCI timeout.
    if (entry.pucchs_to_rx > 0) {
      entry.pucchs_to_rx = 0;
      timeout_notifier.on_timeout(sl_rx, entry.crnti, entry.chosen_action);
    }

    // We remove the entry from the short timeout linked list, but we don't delete the entry as it is still present
    // in the main uci wheel.
    rid                           = entry.next_short_timeout;
    entry.next_short_timeout      = invalid_row_id;
    entry.short_timeout_wheel_pos = {};
  }
  rem_pucch_timeout = invalid_row_id;

  // Handle new UCI grants.
  auto& uci_wheel_sl_tx = uci_wheel[sl_tx.count()];
  ocudu_sanity_check(uci_wheel_sl_tx == invalid_row_id, "The wheel should be empty");
  for (const pucch_info& pucch : result.ul.pucchs) {
    if (pucch.uci_bits.harq_ack_nof_bits == 0) {
      // Only PUCCHs with HARQ-ACK bits need to be buffered for timeout handling.
      continue;
    }

    if (pucch.format() == pucch_format::FORMAT_1) {
      // Check if there is another PUCCH F1 (SR + HARQ-ACK case). If so, increment pucchs_to_rx.
      auto rid = uci_wheel_sl_tx;
      while (rid != invalid_row_id) {
        auto& entry = uci_pool.at<slot_column_id::uci>(rid);
        if (entry.crnti == pucch.crnti) {
          entry.pucchs_to_rx++;
          break;
        }
        rid = entry.next;
      }
      if (rid != invalid_row_id) {
        // Another PUCCH F1 was found in this slot. Avoid adding more than one UCI grant for the same RNTI in the wheel.
        continue;
      }
    }

    // Create new UCI entry and save it in the UCI wheel.
    uci_entry entry;
    entry.crnti        = pucch.crnti;
    entry.pucchs_to_rx = 1;
    // The chosen action set here is what will be propagated in case of timeout.
    entry.chosen_action.harq_ack_bits.resize(pucch.uci_bits.harq_ack_nof_bits);
    entry.next      = uci_wheel_sl_tx;
    soa::row_id rid = uci_pool.insert(entry);
    uci_wheel_sl_tx = rid;
  }
}
