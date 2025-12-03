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
  if (ta_cfg.ta_cmd_offset_threshold >= 0) {
    ues.reserve(MAX_NOF_DU_UES);
    prohibit_ues.reserve(MAX_NOF_DU_UES);
    meas_ues.reserve(MAX_NOF_DU_UES);
  }
}

ue_ta_manager ta_management_system::add_ue(time_alignment_group::id_t     pcell_tag_id,
                                           ue_logical_channel_repository& lc_ch_mgr)
{
  if (ta_cfg.ta_cmd_offset_threshold < 0) {
    // TA management disabled for this UE.
    return ue_ta_manager{*this, invalid_row_id};
  }

  // Create UE context.
  auto row_id = ues.insert(ue_ta_context{&lc_ch_mgr});
  update_tags(row_id, std::array<time_alignment_group::id_t, 1>{pcell_tag_id});

  // Set initial state to prohibit with undefined start time.
  prohibit_ues.push_back(row_id);

  return ue_ta_manager{*this, row_id};
}

void ta_management_system::rem_ue(soa::row_id ue_id)
{
  ue_ta_context& u = ues.at<ue_component::context>(ue_id);
  if (u.state == state_t::prohibit) {
    prohibit_ues.erase(std::remove(prohibit_ues.begin(), prohibit_ues.end(), ue_id), prohibit_ues.end());
  } else {
    meas_ues.erase(std::remove(meas_ues.begin(), meas_ues.end(), ue_id), meas_ues.end());
  }
  ues.erase(ue_id);
}

void ta_management_system::slot_indication(slot_point sl_tx)
{
  // Update UEs in prohibit state, if needed.
  auto exit_prohibit = [this, sl_tx](soa::row_id ue_id) {
    ue_ta_context& u = ues.at<ue_component::context>(ue_id);
    ocudu_sanity_check(u.state == state_t::prohibit, "UE TA manager in invalid state");

    if (not u.start_time.valid() or (sl_tx - u.start_time) > static_cast<int>(ta_cfg.measurement_prohibit_period)) {
      // Move UE to measurement state.
      u.start_time = sl_tx;
      u.state      = state_t::measure;
      meas_ues.push_back(ue_id);
      return true;
    }
    return false;
  };
  prohibit_ues.erase(std::remove_if(prohibit_ues.begin(), prohibit_ues.end(), exit_prohibit), prohibit_ues.end());

  // Update UEs in measurement state, if needed.
  auto exit_measure = [this, sl_tx](soa::row_id ue_id) {
    ue_ta_context& u = ues.at<ue_component::context>(ue_id);
    ocudu_sanity_check(u.state == state_t::measure, "UE TA manager in invalid state");

    // Early return if measurement interval is short.
    if ((sl_tx - u.start_time) < static_cast<int>(ta_cfg.measurement_period)) {
      return false;
    }

    bool ta_cmd_sent = false;
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
        if (u.lc_ch_mgr->handle_mac_ce_indication(
                {.ce_lcid    = lcid_dl_sch_t::TA_CMD,
                 .ce_payload = ta_cmd_ce_payload{.tag_id = tag_id, .ta_cmd = new_t_a}})) {
          ta_cmd_sent = true;
        } else {
          // Early return if queueing the TA CMD indication failed. Will try again at the next slot indication.
          logger.warning("Dropped TA command, queue is full.");
          return false;
        }
      }

      // Reset stored measurements within the measurement window.
      u.n_ta_reports[tag_idx].window_count_samples = 0;
      u.n_ta_reports[tag_idx].window_sum_samples   = 0;
    }

    // Add UE to prohibit list.
    // Note: If no TA CMD was sent, UE goes to prohibit list of duration 0.
    u.state      = state_t::prohibit;
    u.start_time = ta_cmd_sent and ta_cfg.measurement_prohibit_period > 0 ? sl_tx : slot_point{};
    prohibit_ues.push_back(ue_id);
    return true;
  };
  meas_ues.erase(std::remove_if(meas_ues.begin(), meas_ues.end(), exit_measure), meas_ues.end());
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
      std::round(static_cast<float>(n_ta_diff * pow2(to_numerology_value(ul_scs))) / static_cast<float>(16U * 64) +
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

  ue_ta_context& u = ues.at<ue_component::context>(ue_id);
  if (u.state != state_t::measure) {
    return;
  }

  auto it = std::find_if(
      u.n_ta_reports.begin(), u.n_ta_reports.end(), [tag_id](const auto& meas) { return meas.tag_id == tag_id; });
  if (it == u.n_ta_reports.end()) {
    logger.warning("Discarding TA report. Cause: TAG Id {} is not configured", tag_id.value());
    return;
  }

  // Decide whether to filter out outlier using Welford's algorithm and using a exponential average.
  constexpr static unsigned min_samples_for_outlier_detection = 10;
  if (it->count_until_outlier_detection < min_samples_for_outlier_detection) {
    // Note: for small number of samples, outlier detection is not performed.
    it->count_until_outlier_detection++;
  } else if (is_outlier(n_ta_diff_,
                        it->n_ta_diff_averager.get_average_value(),
                        it->n_ta_diff_sq_averager.get_average_value())) {
    // Outlier detected, discard measurement.
    return;
  }

  // Passed outlier detection. Update statistics.
  it->n_ta_diff_averager.push(n_ta_diff_);
  it->n_ta_diff_sq_averager.push(n_ta_diff_ * n_ta_diff_);
  it->window_sum_samples += n_ta_diff_;
  it->window_count_samples++;
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
