/*
 *
 * Copyright 2021-2026 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "ocudu/adt/circular_vector.h"
#include "ocudu/adt/soa_table.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/scheduler/result/sched_result.h"
#include "ocudu/scheduler/scheduler_feedback_handler.h"

namespace ocudu {

/// Action to be taken on the reception of a UCI indication PDU.
struct uci_action {
  enum class pdu_type : uint8_t { pucch_f0f1, pucch_f2f3f4, pusch };

  /// Which type of UCI indication event led to this action.
  pdu_type type = pdu_type::pucch_f0f1;
  /// Whether the decoding of the UCI was successful.
  bool uci_valid = false;
  /// Whether an SR was detected.
  bool sr_detected = false;
  /// HARQ-ACK bits.
  bounded_bitset<MAX_NOF_HARQS> harq_ack_bits;
  std::optional<float>          ul_sinr_dB;
  /// Timing Advance Offset measured for the UE.
  std::optional<phy_time_unit>   time_advance_offset;
  std::optional<csi_report_data> csi;
};

class uci_indication_timeout_notifier
{
public:
  virtual ~uci_indication_timeout_notifier() = default;

  virtual void on_timeout(slot_point sl_rx, rnti_t crnti, const uci_action& action) = 0;
};

class uci_indication_selector
{
public:
  /// \brief Timeout value to use when the PUCCH has been ACKed/NACKed, but it is expecting another PUCCH before being
  /// cleared (implementation-defined).
  static constexpr unsigned SHORT_PUCCH_TIMEOUT_SLOTS = 8U;
  /// \brief Default timeout in slots after which the HARQ process assumes that the CRC/ACK went missing
  /// (implementation-defined).
  static constexpr unsigned DEFAULT_ACK_TIMEOUT_SLOTS = 256U;

  uci_indication_selector(uci_indication_timeout_notifier& timeout_notifier,
                          unsigned                         ack_timeout_slots         = DEFAULT_ACK_TIMEOUT_SLOTS,
                          unsigned                         max_pucch_grants_per_slot = MAX_PUCCH_PDUS_PER_SLOT);

  std::optional<uci_action> handle_uci_ind_pdu(slot_point sl_rx, const uci_indication::uci_pdu& pdu);

  void handle_result(slot_point sl_tx, const sched_result& result);

  /// \brief Called when an error indication is received for a given slot.
  void handle_discarded_pucchs(slot_point sl_tx);

private:
  static constexpr soa::row_id invalid_row_id{std::numeric_limits<uint32_t>::max()};

  struct uci_entry {
    rnti_t crnti = rnti_t::INVALID_RNTI;
    /// Number of UCI PDUs that need to be combined until a decision is made relative to the UCI outcome.
    uint8_t uci_pdus_to_rx = 0;
    /// Buffered action when several UCI PDUs need to be combined.
    uci_action chosen_action;
    /// Next element in the linked list of UCI entries expected for a given slot Rx.
    soa::row_id next = invalid_row_id;
    /// Next element in the linked list of UCI entries that will expire in a given slot if the remaining UCI PDUs
    /// for this grant do not arrive to the scheduler.
    soa::row_id next_short_timeout = invalid_row_id;
    /// Slot at which the UCI entry will expire if the remaining UCI PDUs do not arrive on time.
    slot_point short_timeout_wheel_pos;
  };

  /// Handle UCI grant timeouts.
  void handle_timeouts(slot_point sl_tx);

  std::optional<uci_action> handle_uci_pdu(const uci_indication::uci_pdu& pdu, uci_entry& entry);

  /// Helper to remove UCI entry from its linked list.
  soa::row_id rem_uci_entry(soa::row_id& head, uci_entry* prev_entry, uci_entry& entry);

  const unsigned                   ack_timeout_slots;
  uci_indication_timeout_notifier& timeout_notifier;
  ocudulog::basic_logger&          logger;

  slot_point last_sl_tx;

  enum class slot_column_id { uci };
  soa::table<slot_column_id, uci_entry> uci_pool;

  circular_vector<soa::row_id> uci_wheel;
  circular_vector<soa::row_id> short_timeout_wheel;
};

} // namespace ocudu
