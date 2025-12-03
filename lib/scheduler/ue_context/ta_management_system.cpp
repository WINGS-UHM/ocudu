/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "ta_management_system.h"
#include "logical_channel_system.h"
#include "ocudu/ocudulog/ocudulog.h"
#include "ocudu/ran/du_types.h"

using namespace ocudu;

/// TA command offset for a zero value.
static constexpr int ta_cmd_offset_zero = 31;

ta_management_system::ta_management_system(const scheduler_ta_control_config& ta_cfg_, subcarrier_spacing ul_scs_) :
  ta_cfg(ta_cfg_), ul_scs(ul_scs_), logger(ocudulog::fetch_basic_logger("SCHED"))
{
  ocudu_assert(ta_cfg.measurement_period > 0, "Invalid measurement period");
  if (ta_cfg.ta_cmd_offset_threshold < 0) {
    // TA management disabled.
    return;
  }

  ues.reserve(MAX_NOF_DU_UES);
  time_wheel.resize(ta_cfg.measurement_period + ta_cfg.measurement_prohibit_period);
}

ue_ta_manager ta_management_system::add_ue(time_alignment_group::id_t     pcell_tag_id,
                                           ue_logical_channel_repository& lc_ch_mgr)
{
  if (ta_cfg.ta_cmd_offset_threshold < 0) {
    // TA management disabled for this UE.
    return ue_ta_manager{*this, invalid_row_id};
  }

  // Create UE context.
  auto row_id = ues.insert(ue_ta_context{&lc_ch_mgr}, invalid_row_id);
  update_tags(row_id, std::array<time_alignment_group::id_t, 1>{pcell_tag_id});

  // Push new node to the wheel linked list head.
  auto&        wheel_head = time_wheel[row_id.value() % time_wheel.size()].head;
  soa::row_id& next       = ues.at<ue_component::wheel_next_node>(row_id);
  next                    = wheel_head;
  wheel_head              = row_id;

  return ue_ta_manager{*this, row_id};
}

void ta_management_system::rem_ue(soa::row_id ue_id)
{
  // Remove UE from the wheel.
  soa::row_id next       = ues.at<ue_component::wheel_next_node>(ue_id);
  auto&       wheel_head = time_wheel[ue_id.value() % time_wheel.size()].head;
  soa::row_id node       = wheel_head;
  for (soa::row_id prev = invalid_row_id; node != invalid_row_id;) {
    if (node == ue_id) {
      if (prev == invalid_row_id) {
        // Head node.
        wheel_head = next;
      } else {
        soa::row_id& prev_next = ues.at<ue_component::wheel_next_node>(prev);
        prev_next              = next;
      }
      break;
    }
    prev = node;
    node = ues.at<ue_component::wheel_next_node>(node);
  }
  ocudu_assert(node != invalid_row_id, "UE not found in time wheel");

  // Remove UE from repository.
  ues.erase(ue_id);
}

void ta_management_system::slot_indication(slot_point sl_tx)
{
  // Select a time wheel position based on the current index.
  auto& wheel_head = time_wheel[current_wheel_index % time_wheel.size()].head;
  ++current_wheel_index;

  // Consider that all UEs in the wheel slot have completed their measurements.
  for (soa::row_id ue_id = wheel_head; ue_id != invalid_row_id; ue_id = ues.at<ue_component::wheel_next_node>(ue_id)) {
    ue_ta_context& u = ues.at<ue_component::context>(ue_id);
    handle_ue_ta_cmds(u);
  }
}

void ta_management_system::handle_ue_ta_cmds(ue_ta_context& u)
{
  for (unsigned tag_idx = 0; tag_idx != u.n_ta_reports.size(); ++tag_idx) {
    if (u.n_ta_reports[tag_idx].window_count_samples == 0) {
      // No N_TA update measurements for this TAG ID.
      continue;
    }
    const time_alignment_group::id_t tag_id = u.n_ta_reports[tag_idx].tag_id;

    // Send Timing Advance command only if the offset is equal to or greater than the threshold.
    // The new Timing Advance Command is a value ranging from [0,...,63] as per TS 38.213, clause 4.2. Hence, we
    // need to subtract a value of 31 (as per equation in the same clause) to get the change in Timing Advance
    // Command.
    const unsigned new_t_a = compute_new_t_a(compute_avg_n_ta_difference(u, tag_idx));
    if (abs(static_cast<int>(new_t_a) - ta_cmd_offset_zero) >= ta_cfg.ta_cmd_offset_threshold) {
      // Send Timing Advance Command to UE.
      if (not u.lc_ch_mgr->handle_mac_ce_indication(
              {.ce_lcid    = lcid_dl_sch_t::TA_CMD,
               .ce_payload = ta_cmd_ce_payload{.tag_id = tag_id, .ta_cmd = new_t_a}})) {
        // Early return if queueing the TA CMD indication failed. Will try again in the future.
        logger.warning("Dropped TA command, queue is full.");
        return;
      }
    }

    // Reset stored measurements within the measurement window.
    u.n_ta_reports[tag_idx].window_count_samples = 0;
    u.n_ta_reports[tag_idx].window_sum_samples   = 0;
  }
}

void ta_management_system::update_tags(soa::row_id ue_id, span<const time_alignment_group::id_t> tag_ids)
{
  // TODO: Avoid losing all history.
  auto& ue_ctxt = ues.at<ue_component::context>(ue_id);
  ue_ctxt.n_ta_reports.resize(tag_ids.size());
  for (unsigned i = 0, e = ue_ctxt.n_ta_reports.size(); i != e; ++i) {
    ue_ctxt.n_ta_reports[i].tag_id = tag_ids[i];
    ue_ctxt.n_ta_reports[i].n_ta_diff_averager.reset();
    ue_ctxt.n_ta_reports[i].n_ta_diff_sq_averager.reset();
    ue_ctxt.n_ta_reports[i].count_until_outlier_detection = 0;
    ue_ctxt.n_ta_reports[i].window_sum_samples            = 0;
    ue_ctxt.n_ta_reports[i].window_count_samples          = 0;
  }
}

unsigned ta_management_system::compute_new_t_a(int64_t n_ta_diff)
{
  return static_cast<unsigned>(
      std::round(static_cast<float>(n_ta_diff * get_nof_slots_per_subframe(ul_scs)) / static_cast<float>(16U * 64) +
                 static_cast<float>(ta_cmd_offset_zero) - ta_cfg.target));
}

int64_t ta_management_system::compute_avg_n_ta_difference(const ue_ta_context& ue_ctxt, unsigned tag_idx)
{
  auto& report = ue_ctxt.n_ta_reports[tag_idx];
  return report.window_count_samples > 0 ? report.window_sum_samples / static_cast<int64_t>(report.window_count_samples)
                                         : ta_cmd_offset_zero;
}

/// \brief Determines whether a sample is an outlier based on Welford's algorithm.
static bool is_outlier(double sample, double mean, double sq_mean, double thres = 1.75)
{
  double var     = sq_mean - mean * mean;
  var            = var < 0.0 ? 0.0 : var; // small numerical errors can lead to negative variance
  double std_dev = std::sqrt(var);

  if (std_dev == 0) {
    // There is no spread, so no outlier detection possible. Accept all samples.
    return false;
  }

  // [Implementation-defined] Welford's algorithm z-threshold. Adjust this threshold as needed.
  // There is spread, so we can apply the outlier detection algorithm.
  return std::abs(sample - mean) > thres * std_dev;
}

void ta_management_system::handle_ul_n_ta_update_indication(soa::row_id                ue_id,
                                                            time_alignment_group::id_t tag_id,
                                                            int64_t                    n_ta_diff_,
                                                            float                      ul_sinr)
{
  if (ul_sinr <= ta_cfg.update_measurement_ul_sinr_threshold) {
    // [Implementation-defined] Discard measurement due to low UL SINR.
    // NOTE: From the testing with COTS UE its observed that N_TA update measurements with UL SINR less than 10 dB were
    // majorly outliers.
    return;
  }

  ue_ta_context& u                    = ues.at<ue_component::context>(ue_id);
  const unsigned wheel_diff           = (time_wheel.size() + current_wheel_index - ue_id.value()) % time_wheel.size();
  const bool     is_in_prohibit_state = wheel_diff < ta_cfg.measurement_prohibit_period;
  if (is_in_prohibit_state) {
    // Discard measurement since UE is in prohibit state.
    return;
  }

  // Find respective TAG-ID measurement context.
  auto it = std::find_if(
      u.n_ta_reports.begin(), u.n_ta_reports.end(), [tag_id](const auto& meas) { return meas.tag_id == tag_id; });
  if (it == u.n_ta_reports.end()) {
    logger.warning("Discarding TA report. Cause: TAG Id {} is not configured", tag_id.value());
    return;
  }
  tag_measurement& tag_meas = *it;

  // Decide whether to filter out outlier using Welford's algorithm and using a exponential average.
  constexpr static unsigned min_samples_for_outlier_detection = 10;
  if (tag_meas.count_until_outlier_detection < min_samples_for_outlier_detection) {
    // Note: for small number of samples, outlier detection is not performed.
    tag_meas.count_until_outlier_detection++;
  } else if (is_outlier(n_ta_diff_,
                        tag_meas.n_ta_diff_averager.get_average_value(),
                        tag_meas.n_ta_diff_sq_averager.get_average_value())) {
    // Outlier detected, discard measurement.
    return;
  }

  // Passed outlier detection. Update statistics.
  tag_meas.n_ta_diff_averager.push(n_ta_diff_);
  tag_meas.n_ta_diff_sq_averager.push(n_ta_diff_ * n_ta_diff_);
  tag_meas.window_sum_samples += n_ta_diff_;
  tag_meas.window_count_samples++;
}

ue_ta_manager::~ue_ta_manager()
{
  reset();
}

void ue_ta_manager::reset()
{
  if (parent != nullptr and active()) {
    parent->rem_ue(ue_id);
    parent = nullptr;
  }
}
