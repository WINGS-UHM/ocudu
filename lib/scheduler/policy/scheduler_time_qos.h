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

class ue_history_system
{
  constexpr static soa::row_id invalid_row_id = soa::row_id{std::numeric_limits<uint32_t>::max()};

public:
  class ue_history
  {
    ue_history(const ue_history_system& parent_, soa::row_id rid_) : parent(&parent_), rid(rid_) {}

  public:
    ue_history() = default;

    /// Returns average DL rate expressed in bytes per slot of the UE.
    double dl_avg_rate() const { return parent->ue_db.at<component::dl_avg>(rid); }
    /// Returns average UL rate expressed in bytes per slot of the UE.
    double ul_avg_rate() const { return parent->ue_db.at<component::ul_avg>(rid); }

  private:
    friend class ue_history_system;

    const ue_history_system* parent = nullptr;
    soa::row_id              rid    = invalid_row_id;
  };

  ue_history_system();

  void add_ue(du_ue_index_t ue_index);
  void rem_ue(du_ue_index_t ue_index);

  /// Called once per slot to notify UE contexts about new slot indication.
  void slot_indication(slot_point slot_tx);

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
    // Sum of DL bytes allocated for a given slot, before it is taken into account in the average rate computation.
    accum_dl_samples,
    // Sum of UL bytes allocated for a given slot, before it is taken into account in the average rate computation.
    accum_ul_samples
  };
  soa::table<component, double, double, double, double> ue_db;

  std::vector<soa::row_id> ue_row_ids;
};

/// Time-domain QoS-aware scheduler policy.
class scheduler_time_qos final : public scheduler_policy
{
public:
  scheduler_time_qos(const scheduler_ue_expert_config& expert_cfg_, du_cell_index_t cell_index);

  void add_ue(du_ue_index_t ue_index) override;

  void rem_ue(du_ue_index_t ue_index) override;

  void slot_indication(slot_point slot_tx) override;

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

  // Policy parameters.
  const time_qos_scheduler_config params;
  const du_cell_index_t           cell_index;
  /// Coefficient used to compute exponential moving average.
  const double exp_avg_alpha = 0.01;

  /// Holds the information needed to compute priority of a UE in a priority queue.
  struct ue_ctxt {
    ue_ctxt(du_ue_index_t ue_index_, du_cell_index_t cell_index_, const scheduler_time_qos* parent_);

    /// Computes the priority of the UE to be scheduled in DL based on the QoS and proportional fair metric.
    void compute_dl_prio(const slice_ue& u, slot_point pdcch_slot, slot_point pdsch_slot);
    /// Computes the priority of the UE to be scheduled in UL based on the proportional fair metric.
    void compute_ul_prio(const slice_ue& u, slot_point pdcch_slot, slot_point pusch_slot);

    const du_ue_index_t       ue_index;
    const du_cell_index_t     cell_index;
    const scheduler_time_qos* parent;

    /// DL priority value of the UE.
    double dl_prio = forbid_prio;
    /// UL priority value of the UE.
    double ul_prio = forbid_prio;
  };

  ue_history_system history;

  slotted_id_table<du_ue_index_t, ue_ctxt, MAX_NOF_DU_UES> ue_history_db;

  slot_point last_pdsch_slot;
  slot_point last_pusch_slot;
};

} // namespace ocudu
