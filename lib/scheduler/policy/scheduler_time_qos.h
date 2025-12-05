/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#pragma once

#include "scheduler_policy.h"
#include "ocudu/adt/slotted_array.h"
#include "ocudu/adt/soa_table.h"
#include "ocudu/scheduler/config/scheduler_expert_config.h"
#include "ocudu/support/math/exponential_averager.h"

namespace ocudu {

/// Contains information about a UE past traffic history.
class ue_history_repository
{
  constexpr static soa::row_id invalid_row_id = soa::row_id{std::numeric_limits<uint32_t>::max()};

public:
  /// Traffic history for a single UE.
  class ue_history
  {
    ue_history(const ue_history_repository& parent_, soa::row_id rid_) : parent(&parent_), rid(rid_) {}

  public:
    ue_history() = default;

    /// \brief Returns the UE average DL rate expressed in bytes per "effective" slice slot opportunity.
    ///
    /// "Effective" slice slot opportunity corresponds to a slot when the corresponding slice is eligible for DL
    /// scheduling. So, in practice, we are ignoring TDD UL slots or non-scheduled slices in the computation.
    double dl_avg_rate() const { return parent->ue_db.at<component::dl_avg>(rid); }
    /// \brief Returns the UE average UL rate expressed in bytes per "effective" slice slot opportunity.
    ///
    /// "Effective" slice slot opportunity corresponds to a slot when the corresponding slice is eligible for UL
    /// scheduling. So, in practice, we are ignoring TDD DL slots or non-scheduled slices in the computation.
    double ul_avg_rate() const { return parent->ue_db.at<component::ul_avg>(rid); }

  private:
    friend class ue_history_repository;

    const ue_history_repository* parent = nullptr;
    soa::row_id                  rid    = invalid_row_id;
  };

  ue_history_repository();

  void add_ue(du_ue_index_t ue_index);
  void rem_ue(du_ue_index_t ue_index);

  /// Called to save DL and UL newTx grants allocated to UEs in the given slot.
  void save_dl_newtx_grants(span<const dl_msg_alloc> dl_grants);
  void save_ul_newtx_grants(span<const ul_sched_info> ul_grants);

  ue_history operator[](du_ue_index_t ue_index) const { return {*this, ue_row_ids[ue_index]}; }

private:
  /// Coefficient used to compute exponential moving average.
  static constexpr double exp_avg_alpha = 0.01;

  enum class component {
    // Average DL rate expressed in bytes per slot experienced by UE.
    dl_avg,
    // Average UL rate expressed in bytes per slot experienced by UE.
    ul_avg,
  };

  /// \brief Table holding historical traffic information for all UEs.
  /// We use SoA for better vectorization when updating average rates.
  soa::table<component, float, float> ue_db;

  /// Mapping of UE index to SoA table row IDs.
  std::vector<soa::row_id> ue_row_ids;

  /// Internal buffer to store last DL samples before updating average rates.
  std::vector<float> last_samples_buffer;
};

/// Time-domain QoS-aware scheduler policy.
class scheduler_time_qos final : public scheduler_policy
{
public:
  scheduler_time_qos(const time_qos_scheduler_config& policy_cfg_, du_cell_index_t cell_index);

  void add_ue(du_ue_index_t ue_index) override;

  void rem_ue(du_ue_index_t ue_index) override;

  void compute_ue_dl_priorities(slot_point               pdcch_slot,
                                slot_point               pdsch_slot,
                                span<ue_newtx_candidate> ue_candidates) override;

  void compute_ue_ul_priorities(slot_point               pdcch_slot,
                                slot_point               pusch_slot,
                                span<ue_newtx_candidate> ue_candidates) override;

  void save_dl_newtx_grants(span<const dl_msg_alloc> dl_grants) override;

  void save_ul_newtx_grants(span<const ul_sched_info> ul_grants) override;

private:
  // Value used to flag that the UE cannot be allocated in a given slot.
  static constexpr double forbid_prio = std::numeric_limits<double>::lowest();

  /// Computes the priority of the UE to be scheduled in DL based on the QoS and proportional fair metric.
  [[nodiscard]] ue_sched_priority compute_dl_prio(const slice_ue& u, slot_point pdcch_slot, slot_point pdsch_slot);

  /// Computes the priority of the UE to be scheduled in UL based on the proportional fair metric.
  [[nodiscard]] ue_sched_priority compute_ul_prio(const slice_ue& u, slot_point pdcch_slot, slot_point pusch_slot);

  // Policy parameters.
  const time_qos_scheduler_config params;

  /// Holds historical traffic information for UEs of this slice.
  ue_history_repository ue_history_db;

  slot_point last_pdsch_slot;
  slot_point last_pusch_slot;
};

} // namespace ocudu
