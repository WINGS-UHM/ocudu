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

#include "ocudu/adt/soa_table.h"
#include "ocudu/ocudulog/logger.h"
#include "ocudu/ran/slot_point.h"
#include "ocudu/ran/time_alignment_config.h"
#include "ocudu/scheduler/config/scheduler_expert_config.h"
#include "ocudu/support/math/exponential_averager.h"

namespace ocudu {

class ue_logical_channel_repository;

class ue_ta_manager;

class ta_management_system
{
  /// Maximum number of TAG measurements supported per UE.
  constexpr static size_t      MAX_NOF_TAG_MEASUREMENTS = 4;
  constexpr static soa::row_id invalid_row_id{std::numeric_limits<uint32_t>::max()};

public:
  explicit ta_management_system(const scheduler_ta_control_config& ta_cfg_, subcarrier_spacing ul_scs_);

  /// \brief Adds a new UE to the TA management system.
  ue_ta_manager add_ue(time_alignment_group::id_t pcell_tag_id, ue_logical_channel_repository& lc_ch_mgr_);

  /// \brief Handles Timing Advance adaptation related tasks at slot indication.
  void slot_indication(slot_point sl_tx);

  /// Number of active UEs in the TA management system.
  unsigned nof_active_ues() const { return static_cast<unsigned>(ues.size()); }

private:
  friend class ue_ta_manager;
  static constexpr double n_ta_diff_avg_decay = 0.01;

  /// State of the Timing Advance manager.
  enum class state_t : uint8_t {
    /// Performing measurements.
    measure,
    /// Prohibit state.
    prohibit
  };

  struct tag_measurement {
    time_alignment_group::id_t     tag_id;
    exp_average_fast_start<double> n_ta_diff_averager{n_ta_diff_avg_decay};
    exp_average_fast_start<double> n_ta_diff_sq_averager{n_ta_diff_avg_decay};
    uint32_t                       count_until_outlier_detection = 0;
    /// Number of samples within the current measurement window.
    uint32_t window_count_samples = 0;
    /// Sum of samples within the current measurement window.
    int64_t window_sum_samples = 0;
  };

  struct ue_ta_context {
    ue_logical_channel_repository* lc_ch_mgr = nullptr;
    /// State of the Timing Advance manager.
    state_t state = state_t::prohibit;
    /// Starting point of the measurement/prohibit interval (depending on state).
    /// \remark In case state == prohibit and not \c start_time.valid(), prohibit period has duration 0.
    slot_point start_time;
    /// List of N_TA update (N_TA_new - N_TA_old value in T_C units) measurements maintained per Timing Advance Group.
    /// The array index corresponds to TAG ID. And, the corresponding array value (i.e. vector) holds N_TA update
    /// measurements for that TAG ID.
    static_vector<tag_measurement, MAX_NOF_TAG_MEASUREMENTS> n_ta_reports;
  };

  /// Remove a UE from the TA management system.
  void rem_ue(soa::row_id ue_id);

  void handle_ul_n_ta_update_indication(soa::row_id                ue_id,
                                        time_alignment_group::id_t tag_id,
                                        int64_t                    n_ta_diff_,
                                        float                      ul_sinr);

  /// \brief Computes the average of N_TA update measurements.
  static int64_t compute_avg_n_ta_difference(const ue_ta_context& ue_ctxt, unsigned tag_idx);

  /// \brief Computes new Timing Advance Command value (T_A) as per TS 38.213, clause 4.2.
  /// \return Timing Advance Command value. Values [0,...,63].
  unsigned compute_new_t_a(int64_t n_ta_diff);

  void update_tags(soa::row_id ue_id, span<const time_alignment_group::id_t> tag_ids);

  const scheduler_ta_control_config ta_cfg;
  const subcarrier_spacing          ul_scs;
  ocudulog::basic_logger&           logger;

  enum class ue_component { context };
  soa::table<ue_component, ue_ta_context> ues;

  /// UEs in prohibit state.
  std::vector<soa::row_id> prohibit_ues;

  /// UEs in measurement state.
  std::vector<soa::row_id> meas_ues;
};

/// Handle to the TA manager of a single UE.
class ue_ta_manager
{
  ue_ta_manager(ta_management_system& parent_, soa::row_id ue_id_) : parent(&parent_), ue_id(ue_id_) {}

public:
  ue_ta_manager()                     = default;
  ue_ta_manager(const ue_ta_manager&) = delete;
  ue_ta_manager(ue_ta_manager&& other) noexcept :
    parent(std::exchange(other.parent, nullptr)),
    ue_id(std::exchange(other.ue_id, ta_management_system::invalid_row_id))
  {
  }
  ue_ta_manager& operator=(const ue_ta_manager&) = delete;
  ue_ta_manager& operator=(ue_ta_manager&& other) noexcept
  {
    reset();
    parent = std::exchange(other.parent, nullptr);
    ue_id  = std::exchange(other.ue_id, ta_management_system::invalid_row_id);
    return *this;
  }
  ~ue_ta_manager();

  /// \brief Resets the TA manager, disabling TA management for the UE.
  void reset();

  /// Whether the TA management is active for the UE.
  bool active() const { return ue_id != ta_management_system::invalid_row_id; }

  /// \brief Updates the list of configured TAG IDs for the UE.
  void update_tags(span<const time_alignment_group::id_t> tag_ids)
  {
    ocudu_sanity_check(parent != nullptr, "TA manager is not in valid state");
    if (active()) {
      parent->update_tags(ue_id, tag_ids);
    }
  }

  /// \brief Handles N_TA update indication.
  void handle_ul_n_ta_update_indication(time_alignment_group::id_t tag_id, int64_t n_ta_diff_, float ul_sinr)
  {
    ocudu_sanity_check(parent != nullptr, "TA manager is not in valid state");
    if (active()) {
      parent->handle_ul_n_ta_update_indication(ue_id, tag_id, n_ta_diff_, ul_sinr);
    }
  }

private:
  friend class ta_management_system;

  ta_management_system* parent = nullptr;
  soa::row_id           ue_id{ta_management_system::invalid_row_id};
};

} // namespace ocudu
