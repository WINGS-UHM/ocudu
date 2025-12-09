/*
 *
 * Copyright 2021-2025 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

#include "logical_channel_system.h"
#include "ocudu/scheduler/scheduler_feedback_handler.h"

using namespace ocudu;

/// (Implementation-defined) Estimation of how much space the MAC should leave for RLC segmentation header overhead.
static constexpr unsigned RLC_SEGMENTATION_OVERHEAD = 3;

/// Estimation of the space required for MAC SDU (including RLC overhead) + MAC subheader
static constexpr unsigned get_mac_sdu_with_subhdr_and_rlc_hdr_estim(lcid_t lcid, unsigned payload)
{
  const unsigned rlc_ovh = (lcid != LCID_SRB0 && payload > 0) ? RLC_SEGMENTATION_OVERHEAD : 0;
  return get_mac_sdu_required_bytes(payload + rlc_ovh);
}

/// Determine the MAC SDU size from the total size of MAC SDU + MAC subheader.
static unsigned get_mac_sdu_size(unsigned sdu_and_subheader_bytes)
{
  if (sdu_and_subheader_bytes == 0) {
    return 0;
  }
  const unsigned sdu_size = sdu_and_subheader_bytes - MIN_MAC_SDU_SUBHEADER_SIZE;
  return sdu_size < MAC_SDU_SUBHEADER_LENGTH_THRES ? sdu_size : sdu_size - 1;
}

logical_channel_system_utils::logical_channel_mapper::logical_channel_mapper() :
  lookup_ue_lcid_to_row_id(MAX_NOF_RB_LCIDS * MAX_NOF_DU_UES, invalid_row_id)
{
  /// (Implementation-defined) Pre-reserved number of DRBs per UE for QoS tracking.
  static constexpr unsigned PRERESERVED_NOF_DRBS_PER_UE = 2;
  qos_channels.reserve(PRERESERVED_NOF_DRBS_PER_UE * MAX_NOF_DU_UES);
  fields.reserve(MAX_NOF_RB_LCIDS * MAX_NOF_DU_UES);
}

void logical_channel_system_utils::logical_channel_mapper::slot_indication()
{
  // Update the bit rates of the UE logical channels with tracked bit rates.
  for (auto row_view : qos_channels) {
    auto& item = row_view.at<qos_context>();
    item.dl_avg_bytes_per_slot.push(item.dl_last_sched_bytes);
    item.dl_last_sched_bytes = 0;
  }
}

soa::row_id logical_channel_system_utils::logical_channel_mapper::add_lc(du_ue_index_t ue_index, lcid_t lcid)
{
  size_t index = get_index(ue_index, lcid);
  ocudu_assert(lookup_ue_lcid_to_row_id[index] == invalid_row_id, "duplicate UE LCID registration");
  soa::row_id rid                 = fields.insert(true, units::bytes{0}, invalid_slice_id, std::nullopt, slot_point{});
  lookup_ue_lcid_to_row_id[index] = static_cast<uint16_t>(rid.value());
  return rid;
}

void logical_channel_system_utils::logical_channel_mapper::rem_lc(du_ue_index_t ue_index, lcid_t lcid)
{
  const size_t index = get_index(ue_index, lcid);
  soa::row_id  rid   = soa::row_id{lookup_ue_lcid_to_row_id[index]};
  ocudu_assert(rid.value() != invalid_row_id, "Invalid UE LCID deregistration");

  // Detach LC from QoS tracking, if active.
  if (auto qrow = qos_row(rid); qrow.has_value()) {
    qos_channels.erase(*qrow);
  }

  // Remove LC entry.
  fields.erase(rid);
  lookup_ue_lcid_to_row_id[index] = invalid_row_id;
}

void logical_channel_system_utils::logical_channel_mapper::register_lc_slice(soa::row_id    rid,
                                                                             ran_slice_id_t dl_slice_id)
{
  ocudu_sanity_check(rid.value() != invalid_row_id, "UE LCID not found when registering slice");
  fields.at<column_type::dl_slice_id>(rid) = dl_slice_id;
}

void logical_channel_system_utils::logical_channel_mapper::deregister_lc_slice(soa::row_id rid)
{
  ocudu_sanity_check(rid.value() != invalid_row_id, "UE LCID not found when registering slice");
  fields.at<column_type::dl_slice_id>(rid) = invalid_slice_id;
}

void logical_channel_system_utils::logical_channel_mapper::enable_qos_tracking(soa::row_id lc_rid,
                                                                               unsigned    window_size_slots)
{
  auto& dl_qos_row = qos_row(lc_rid);
  if (not dl_qos_row.has_value()) {
    // New GBR QoS, initiate tracking.
    dl_qos_row = qos_channels.insert(qos_context{moving_averager<unsigned>{}});
  }
  qos_channels.row(*dl_qos_row).at<qos_context>().dl_avg_bytes_per_slot.resize(window_size_slots);
}

void logical_channel_system_utils::logical_channel_mapper::disable_qos_tracking(soa::row_id lc_rid)
{
  auto& dl_qos_row = qos_row(lc_rid);
  if (dl_qos_row.has_value()) {
    // Remove from tracked QoS channels.
    qos_channels.erase(*dl_qos_row);
    dl_qos_row = std::nullopt;
  }
}

void logical_channel_system_utils::logical_channel_mapper::deactivate(soa::row_id lc_rid)
{
  uint8_t& active_flag = fields.at<column_type::active>(lc_rid);
  if (active_flag == 0) {
    return;
  }

  // Eliminate associated QoS being tracked.
  if (auto& qrow = qos_row(lc_rid); qrow.has_value()) {
    qos_channels.erase(*qrow);
  }

  // Detach from RAN slice.
  // Note: pending bytes will be cleared at the end of this function.
  deregister_lc_slice(lc_rid);

  // Flag LCID as disabled.
  active_flag = 0;
}

unsigned* logical_channel_system_utils::logical_channel_mapper::last_sched_bytes(soa::row_id lc_rid)
{
  auto& qos_row_opt = qos_row(lc_rid);
  if (qos_row_opt.has_value()) {
    return &qos_channels.row(*qos_row_opt).at<qos_context>().dl_last_sched_bytes;
  }
  return nullptr;
}

double logical_channel_system_utils::logical_channel_mapper::avg_dl_bits_per_slot(soa::row_id lc_rid) const
{
  auto& qos_row_opt = qos_row(lc_rid);
  if (qos_row_opt.has_value()) {
    const auto& item = qos_channels.row(*qos_row_opt).at<qos_context>();
    return item.dl_avg_bytes_per_slot.average() * 8;
  }
  return 0.0;
}

logical_channel_system::logical_channel_system()
{
  static constexpr unsigned PRERESERVED_NOF_CES = MAX_NOF_DU_UES;
  static constexpr unsigned PRERESERVED_SLICES  = 4;

  // Pre-reserve space to avoid allocations in latency-critical path.
  pending_ces.reserve(PRERESERVED_NOF_CES);
  slices.reserve(PRERESERVED_SLICES);
  ues.reserve(MAX_NOF_DU_UES);
}

void logical_channel_system::slot_indication()
{
  lc_mapper.slot_indication();
}

ue_logical_channel_repository logical_channel_system::create_ue(du_ue_index_t                   ue_index,
                                                                subcarrier_spacing              scs_common,
                                                                bool                            starts_in_fallback,
                                                                logical_channel_config_list_ptr log_channels_configs)
{
  ocudu_assert(configured_ues.size() <= ue_index or not configured_ues.test(ue_index), "duplicate UE index");
  // Creates a UE entry in the table.
  soa::row_id ue_rid =
      ues.insert(ue_config_context{}, ue_context{}, ue_ul_context{}, ue_channel_context{}, ue_ul_channel_context{});
  auto row                                   = ues.row(ue_rid);
  row.at<ue_context>().fallback_state        = starts_in_fallback;
  row.at<ue_config_context>().ue_index       = ue_index;
  row.at<ue_config_context>().slots_per_msec = get_nof_slots_per_subframe(scs_common);
  if (configured_ues.size() <= ue_index) {
    // Need to resize UE bitsets.
    configured_ues.resize(ue_index + 1);
    ues_with_pending_ces.resize(ue_index + 1);
    ues_with_pending_sr.resize(ue_index + 1);
    pending_dl_ue_lcids.resize((ue_index + 1) * MAX_NOF_RB_LCIDS);
    for (auto it = slices.begin(); it != slices.end(); ++it) {
      it->second.pending_dl_ues.resize(ue_index + 1);
      it->second.pending_ul_ues.resize(ue_index + 1);
    }
  }
  configured_ues.set(ue_index);
  if (log_channels_configs.has_value()) {
    // Apply LC configuration.
    configure(ue_rid, log_channels_configs);
  }

  // Return handle to UE logical channel repository.
  return ue_logical_channel_repository{*this, ue_index, ue_rid};
}

size_t logical_channel_system::nof_logical_channels() const
{
  size_t count = 0;
  for (const_ue_row u : ues) {
    count += u.at<ue_channel_context>().channels.size();
  }
  return count;
}

bounded_bitset<MAX_NOF_DU_UES> logical_channel_system::get_ues_with_dl_pending_data(ran_slice_id_t slice_id) const
{
  auto slice_it = slices.find(slice_id);
  if (slice_it == slices.end()) {
    // This slice is not configured.
    return bounded_bitset<MAX_NOF_DU_UES>{};
  }
  if (slice_id != SRB_RAN_SLICE_ID) {
    return slice_it->second.pending_dl_ues;
  }
  return slice_it->second.pending_dl_ues | ues_with_pending_ces;
}

bounded_bitset<MAX_NOF_DU_UES> logical_channel_system::get_ues_with_ul_pending_data(ran_slice_id_t slice_id) const
{
  auto slice_it = slices.find(slice_id);
  if (slice_it == slices.end()) {
    // This slice is not configured.
    return bounded_bitset<MAX_NOF_DU_UES>{};
  }
  if (slice_id != SRB_RAN_SLICE_ID) {
    return slice_it->second.pending_ul_ues;
  }
  return slice_it->second.pending_ul_ues | ues_with_pending_sr;
}

void ue_logical_channel_repository::set_fallback_state(bool enter_fallback)
{
  auto        u      = get_ue_row();
  ue_context& ue_ctx = u.at<ue_context>();
  if (ue_ctx.fallback_state == enter_fallback) {
    // no-op.
    return;
  }
  // Fallback state toggled.
  ue_ctx.fallback_state = enter_fallback;

  // Update pending newTx data for all UE slices.
  auto  ueidx     = u.at<ue_config_context>().ue_index;
  auto& ue_ul_ctx = u.at<ue_ul_context>();
  if (ue_ctx.fallback_state) {
    // Entering fallback mode, clear all pending data in slices.
    for (auto it = ue_ctx.pending_bytes_per_slice.begin(), it_end = ue_ctx.pending_bytes_per_slice.end(); it != it_end;
         ++it) {
      if (it->second > 0) {
        it->second = 0;
        parent->slices.at(it->first).pending_dl_ues.reset(ueidx);
      }
    }
    parent->ues_with_pending_ces.reset(ueidx);
    for (auto it = ue_ul_ctx.pending_bytes_per_slice.begin(), it_end = ue_ul_ctx.pending_bytes_per_slice.end();
         it != it_end;
         ++it) {
      if (it->second > 0) {
        it->second = 0;
        parent->slices.at(it->first).pending_ul_ues.reset(ueidx);
      }
    }
  } else {
    // Exiting fallback mode, recompute pending data in slices.
    const auto& ue_ch = u.at<ue_channel_context>();
    for (const auto& [lcid, lc_rid] : ue_ch.channels) {
      parent->on_single_channel_buf_st_update(u,
                                              parent->lc_mapper.active(lc_rid),
                                              parent->lc_mapper.dl_slice_id(lc_rid),
                                              parent->lc_mapper.dl_buf_st(lc_rid).value(),
                                              0);
    }
    if (ue_ch.pending_ces.has_value()) {
      parent->ues_with_pending_ces.set(ueidx);
    }
    auto& ue_ul_ch = u.at<ue_ul_channel_context>();
    for (const auto& [lcgid, ch] : ue_ul_ch.lcgs) {
      parent->on_single_lcg_buf_st_update(u, ch.active, lcgid, ch.slice_id, ch.buf_st, 0);
    }
  }
}

/// Helper to retrieve logical channel priority.
static uint16_t get_lc_prio(const logical_channel_config& cfg)
{
  uint16_t prio = 0;
  if (is_srb(cfg.lcid)) {
    prio = cfg.lcid <= LCID_SRB1 ? 0 : 1;
  } else {
    prio = cfg.qos.has_value() ? cfg.qos->qos.priority.value() * cfg.qos->arp_priority.value()
                               : qos_prio_level_t::max() * arp_prio_level_t::max();
  }
  return prio;
}

void logical_channel_system::remove_ue(soa::row_id ue_rid)
{
  ocudu_assert(ues.has_row_id(ue_rid), "trying to remove non-existing UE");
  const auto ue_index = ues.row(ue_rid).at<ue_config_context>().ue_index;
  ocudu_assert(configured_ues.size() > ue_index and configured_ues.test(ue_index), "duplicate UE index");
  // Disable any slicing, QoS and CE tracking.
  deactivate(ue_rid);

  // Remove all logical channels.
  auto& ue_ch = ues.row(ue_rid).at<ue_channel_context>();
  for (const auto& [lcid, lc_rid] : ue_ch.channels) {
    lc_mapper.rem_lc(ue_index, lcid);
  }

  // Destroy UE context.
  ues.erase(ue_rid);

  // Update UE config bitset (strip rightmost zeros).
  // Note: if return of highest is -1 (not found), the new size will be 0.
  configured_ues.reset(ue_index);
  const auto new_size = static_cast<size_t>(configured_ues.find_highest(true) + 1);
  if (configured_ues.size() != new_size) {
    configured_ues.resize(new_size);
    ues_with_pending_ces.resize(new_size);
    ues_with_pending_sr.resize(new_size);
    pending_dl_ue_lcids.resize(new_size * MAX_NOF_RB_LCIDS);
    for (auto it = slices.begin(), it_end = slices.end(); it != it_end; ++it) {
      it->second.pending_dl_ues.resize(new_size);
      it->second.pending_ul_ues.resize(new_size);
    }
  }
}

void logical_channel_system::configure(soa::row_id ue_rid, logical_channel_config_list_ptr log_channels_configs)
{
  auto  u        = ues.row(ue_rid);
  auto& ue_cfg   = u.at<ue_config_context>();
  auto& ue_ch    = u.at<ue_channel_context>();
  auto& ue_ul_ch = u.at<ue_ul_channel_context>();

  // Exchange new config.
  auto old_cfgs          = ue_cfg.channel_configs;
  ue_cfg.channel_configs = log_channels_configs;

  // If a previously custom configured LC is not in the list of new configs, we delete it.
  // Note: LCID will be removed from sorted_channels later.
  if (old_cfgs.has_value()) {
    for (const auto& old_lc : *old_cfgs) {
      if (not ue_cfg.channel_configs->contains(old_lc->lcid)) {
        // LCID got removed.

        // Detach from RAN slice.
        deregister_lc_ran_slice(ue_rid, old_lc->lcid);

        // Delete the LC context.
        ue_ch.channels.erase(old_lc->lcid);
        lc_mapper.rem_lc(ue_cfg.ue_index, old_lc->lcid);

        // Update LCG context.
        ul_logical_channel_group_context& ul_ch_ctx = ue_ul_ch.lcgs.at(old_lc->lc_group);
        if (--ul_ch_ctx.lc_count == 0) {
          // No more LCIDs in this LCG, remove it.
          remove_lcg(ue_rid, ue_ul_ch, old_lc->lc_group, ul_ch_ctx);
        }
      }
    }
  }

  // Set new LC configurations.
  for (logical_channel_config_ptr ch_cfg : *ue_cfg.channel_configs) {
    const bool new_ch = not ue_ch.channels.contains(ch_cfg->lcid);
    if (new_ch) {
      // New LCID, create context.
      auto lc_rid = lc_mapper.add_lc(ue_cfg.ue_index, ch_cfg->lcid);
      ue_ch.channels.emplace(ch_cfg->lcid, lc_rid);
    }

    const bool new_lcg = not ue_ul_ch.lcgs.contains(ch_cfg->lc_group);
    if (new_lcg) {
      // New LCG, create context.
      ue_ul_ch.lcgs.emplace(ch_cfg->lc_group, ul_logical_channel_group_context{.lc_count = 1});
    } else {
      if (new_ch) {
        // New LCID on existing LCG.
        ue_ul_ch.lcgs.at(ch_cfg->lc_group).lc_count++;
      } else if (old_cfgs.has_value() and old_cfgs.value()[ch_cfg->lcid]->lc_group != ch_cfg->lc_group) {
        // Existing LCID but the LCG changed.
        ue_ul_ch.lcgs.at(ch_cfg->lc_group).lc_count++;
        // Old LCG loses one LCID.
        lcg_id_t                          old_lcg_id    = old_cfgs.value()[ch_cfg->lcid]->lc_group;
        ul_logical_channel_group_context& old_ul_ch_ctx = ue_ul_ch.lcgs.at(old_lcg_id);
        if (--old_ul_ch_ctx.lc_count == 0) {
          remove_lcg(ue_rid, ue_ul_ch, ch_cfg->lc_group, old_ul_ch_ctx);
        }
      }
    }
    soa::row_id                       dl_lc_rid = ue_ch.channels.at(ch_cfg->lcid);
    ul_logical_channel_group_context& ul_ch_ctx = ue_ul_ch.lcgs.at(ch_cfg->lc_group);

    // Implicitly set channel as active when configured.
    if (not lc_mapper.active(dl_lc_rid)) {
      lc_mapper.activate(dl_lc_rid);
      if (auto slice_id = lc_mapper.dl_slice_id(dl_lc_rid); slice_id.has_value()) {
        // Update slice pending bytes, because we are activating the logical channel.
        on_single_channel_buf_st_update(u, true, slice_id, lc_mapper.dl_buf_st(dl_lc_rid).value(), 0);
      }
    }

    // Implicitly set LCG as active when at least one LCID is active.
    if (not ul_ch_ctx.active and ul_ch_ctx.slice_id.has_value()) {
      // Update slice pending bytes, because we are activating the logical channel.
      on_single_lcg_buf_st_update(u, true, ch_cfg->lc_group, ul_ch_ctx.slice_id, ul_ch_ctx.buf_st, 0);
    }
    ul_ch_ctx.active = true;

    // Handle GBR QoS tracking.
    const bool qos_is_tracked =
        not is_srb(ch_cfg->lcid) and ch_cfg->qos.has_value() and ch_cfg->qos.value().gbr_qos_info.has_value();
    if (qos_is_tracked) {
      const unsigned win_size_msec = ch_cfg->qos.value().qos.average_window_ms.value();

      // Ensure QoS tracking is enabled for DL.
      lc_mapper.enable_qos_tracking(dl_lc_rid, win_size_msec * ue_cfg.slots_per_msec);

      // Ensure QoS tracking is enabled for UL.
      if (not ul_ch_ctx.qos_row.has_value()) {
        // New GBR QoS, initiate tracking.
        auto qos_rid      = qos_lcgs.insert(lcg_qos_context{});
        ul_ch_ctx.qos_row = qos_rid;
      }
      qos_lcgs.row(*ul_ch_ctx.qos_row)
          .at<lcg_qos_context>()
          .ul_avg_bytes_per_slot.resize(win_size_msec * ue_cfg.slots_per_msec);
    } else {
      // Remove from QoS tracking.
      lc_mapper.disable_qos_tracking(dl_lc_rid);

      // TODO: Ensure all LCIDs associated with this LCG are non-GBR before removing LCG QoS tracking.
      if (ul_ch_ctx.qos_row.has_value()) {
        qos_lcgs.erase(*ul_ch_ctx.qos_row);
        ul_ch_ctx.qos_row.reset();
      }
    }
  }

  // Refresh sorted channels list.
  ue_ch.sorted_channels.clear();
  for (const auto& lc_cfg : *ue_cfg.channel_configs) {
    ue_ch.sorted_channels.push_back(lc_cfg->lcid);
  }
  std::sort(ue_ch.sorted_channels.begin(), ue_ch.sorted_channels.end(), [&ue_cfg](lcid_t lhs, lcid_t rhs) {
    return get_lc_prio(*ue_cfg.channel_configs.value()[lhs]) < get_lc_prio(*ue_cfg.channel_configs.value()[rhs]);
  });
}

void logical_channel_system::deactivate(soa::row_id ue_rid)
{
  auto  u         = ues.row(ue_rid);
  auto  ue_idx    = u.at<ue_config_context>().ue_index;
  auto& ue_ch     = u.at<ue_channel_context>();
  auto& ue_ctx    = u.at<ue_context>();
  auto& ue_ul_ch  = u.at<ue_ul_channel_context>();
  auto& ue_ul_ctx = u.at<ue_ul_context>();

  // Clear UE pending CEs.
  ue_ctx.pending_con_res_id = false;
  for (std::optional<soa::row_id> ce_rid = ue_ch.pending_ces; ce_rid.has_value();) {
    auto next = pending_ces.at<mac_ce_context>(*ce_rid).next_ue_ce;
    pending_ces.erase(*ce_rid);
    ce_rid = next;
  }
  ue_ch.pending_ces       = std::nullopt;
  ue_ctx.pending_ce_bytes = 0;
  ues_with_pending_ces.reset(ue_idx);

  // Reset any pending SR.
  ues_with_pending_sr.reset(ue_idx);

  // Reset pending DL LCIDs.
  pending_dl_ue_lcids.fill(ue_idx * MAX_NOF_RB_LCIDS, (ue_idx + 1) * MAX_NOF_RB_LCIDS, false);

  // Deactivate (not erase) all bearers.
  for (const auto& [lcid, lc_rid] : ue_ch.channels) {
    // Disable bearer.
    // Note: pending bytes will be cleared at the end of this function.
    lc_mapper.deactivate(lc_rid);
  }
  for (const auto& [lcgid, ul_ch_ctx] : ue_ul_ch.lcgs) {
    if (ul_ch_ctx.qos_row.has_value()) {
      // Eliminate associated QoS being tracked.
      qos_lcgs.erase(*ul_ch_ctx.qos_row);
    }

    // Detach from RAN slice.
    // Note: pending bytes will be cleared at the end of this function.
    ul_ch_ctx.slice_id.reset();

    // Disable bearer.
    ul_ch_ctx.active = false;
  }

  // Remove slices.
  for (const auto& [slice_id, bytes] : ue_ctx.pending_bytes_per_slice) {
    if (bytes > 0) {
      slices.at(slice_id).pending_dl_ues.reset(ue_idx);
    }
  }
  ue_ctx.pending_bytes_per_slice.clear();
  for (const auto& [slice_id, bytes] : ue_ul_ctx.pending_bytes_per_slice) {
    if (bytes > 0) {
      slices.at(slice_id).pending_ul_ues.reset(ue_idx);
    }
  }
  ue_ul_ctx.pending_bytes_per_slice.clear();

  // Clear channel lookups.
  ue_ch.sorted_channels.clear();
}

void logical_channel_system::set_lcid_ran_slice(soa::row_id ue_rid, lcid_t lcid, ran_slice_id_t slice_id)
{
  auto        u      = ues.row(ue_rid);
  auto&       ue_ch  = u.at<ue_channel_context>();
  ue_context& ue_ctx = u.at<ue_context>();
  ocudu_assert(ue_ch.channels.contains(lcid), "LCID not configured");

  soa::row_id lc_rid    = ue_ch.channels.at(lcid);
  auto        cur_slice = lc_mapper.dl_slice_id(lc_rid);
  if (cur_slice == slice_id) {
    // No-op.
    return;
  }

  // Remove LCID from previous slice.
  deregister_lc_ran_slice(ue_rid, lcid);

  // If slice does not exist yet for the UE, create it.
  auto slice_it = ue_ctx.pending_bytes_per_slice.find(slice_id);
  if (slice_it == ue_ctx.pending_bytes_per_slice.end()) {
    // New slice, add to the list.
    auto data_slice_it = slices.find(slice_id);
    if (data_slice_it == slices.end()) {
      // Create slice if not existing yet.
      slices.emplace(slice_id,
                     ran_slice_context{bounded_bitset<MAX_NOF_DU_UES>(configured_ues.size()),
                                       bounded_bitset<MAX_NOF_DU_UES>(configured_ues.size())});
    }
    ue_ctx.pending_bytes_per_slice.emplace(slice_id, 0);
  }

  // Add LCID to new slice.
  lc_mapper.register_lc_slice(lc_rid, slice_id);

  // Update pending bytes for the slice.
  on_single_channel_buf_st_update(u, lc_mapper.active(lc_rid), slice_id, lc_mapper.dl_buf_st(lc_rid).value(), 0);
}

void logical_channel_system::set_lcg_ran_slice(soa::row_id ue_rid, lcg_id_t lcgid, ran_slice_id_t slice_id)
{
  auto  u      = ues.row(ue_rid);
  auto& ue_ch  = u.at<ue_ul_channel_context>();
  auto& ue_ctx = u.at<ue_ul_context>();
  ocudu_assert(ue_ch.lcgs.contains(lcgid), "LCG-ID not configured");

  ul_logical_channel_group_context& ch_ctx = ue_ch.lcgs.at(lcgid);
  if (ch_ctx.slice_id == slice_id) {
    // No-op.
    return;
  }

  // Remove LCG-ID from previous slice.
  deregister_lcg_ran_slice(ue_rid, lcgid);

  // If slice does not exist yet for the UE, create it.
  auto slice_it = ue_ctx.pending_bytes_per_slice.find(slice_id);
  if (slice_it == ue_ctx.pending_bytes_per_slice.end()) {
    // New slice, add to the list.
    auto data_slice_it = slices.find(slice_id);
    if (data_slice_it == slices.end()) {
      // Create slice if not existing yet.
      slices.emplace(slice_id,
                     ran_slice_context{bounded_bitset<MAX_NOF_DU_UES>(configured_ues.size()),
                                       bounded_bitset<MAX_NOF_DU_UES>(configured_ues.size())});
    }
    ue_ctx.pending_bytes_per_slice.emplace(slice_id, 0);
  }

  // Add LCG-ID to new slice.
  ch_ctx.slice_id = slice_id;

  // Update pending bytes for the slice.
  on_single_lcg_buf_st_update(u, ch_ctx.active, lcgid, ch_ctx.slice_id, ch_ctx.buf_st, 0);
}

void logical_channel_system::deregister_lc_ran_slice(soa::row_id ue_rid, lcid_t lcid)
{
  auto  u     = get_ue(ue_rid);
  auto& ue_ch = u.at<ue_channel_context>();
  if (ue_ch.channels.contains(lcid)) {
    soa::row_id lc_rid        = ue_ch.channels.at(lcid);
    auto        prev_slice_id = lc_mapper.dl_slice_id(lc_rid);
    on_single_channel_buf_st_update(u, lc_mapper.active(lc_rid), prev_slice_id, 0, lc_mapper.dl_buf_st(lc_rid).value());
    if (prev_slice_id.has_value()) {
      // Remove slice from LC.
      lc_mapper.deregister_lc_slice(lc_rid);
      // If it is the last LC attached to the slice, remove slice.
      if (std::none_of(ue_ch.channels.begin(), ue_ch.channels.end(), [this, slid = *prev_slice_id](const auto& ch) {
            return lc_mapper.dl_slice_id(ch.second) == slid;
          })) {
        // No other LC is still attached to the slice for this UE. Remove slice from UE.
        auto& ue_ctxt = u.at<ue_context>();
        ue_ctxt.pending_bytes_per_slice.erase(*prev_slice_id);
      }
    }
  }
}

void logical_channel_system::deregister_lcg_ran_slice(soa::row_id ue_rid, lcg_id_t lcgid)
{
  auto  u     = get_ue(ue_rid);
  auto& ue_ch = u.at<ue_ul_channel_context>();
  auto  lcgit = ue_ch.lcgs.find(lcgid);
  if (lcgit == ue_ch.lcgs.end()) {
    return;
  }
  ul_logical_channel_group_context& ch_ctx = lcgit->second;
  on_single_lcg_buf_st_update(u, ch_ctx.active, lcgid, ch_ctx.slice_id, 0, ch_ctx.buf_st);
  if (ch_ctx.slice_id.has_value()) {
    // Remove slice from LC.
    auto prev_slice_id = *ch_ctx.slice_id;
    ch_ctx.slice_id.reset();
    // If it is the last LC attached to the slice, remove slice.
    if (std::none_of(ue_ch.lcgs.begin(), ue_ch.lcgs.end(), [prev_slice_id](const auto& ch) {
          return ch.second.slice_id == prev_slice_id;
        })) {
      // No other LC is still attached to the slice for this UE. Remove slice from UE.
      auto& ue_ctxt = u.at<ue_context>();
      ue_ctxt.pending_bytes_per_slice.erase(prev_slice_id);
    }
  }
}

void logical_channel_system::handle_dl_buffer_status_indication(soa::row_id ue_row_id,
                                                                lcid_t      lcid,
                                                                unsigned    buffer_status,
                                                                slot_point  hol_toa)
{
  // We apply this limit to avoid potential overflows.
  static constexpr unsigned max_buffer_status = 1U << 24U;
  ocudu_sanity_check(lcid < MAX_NOF_RB_LCIDS, "Max LCID value 32 exceeded");
  auto  u     = get_ue(ue_row_id);
  auto& ue_ch = u.at<ue_channel_context>();
  if (ue_ch.channels.contains(lcid)) {
    soa::row_id lc_rid        = ue_ch.channels.at(lcid);
    auto&       buf_st        = lc_mapper.dl_buf_st(lc_rid);
    auto        prev_buf_st   = buf_st.value();
    buf_st                    = units::bytes{std::min(buffer_status, max_buffer_status)};
    lc_mapper.hol_toa(lc_rid) = hol_toa;
    if ((prev_buf_st > 0) != (buf_st.value() > 0)) {
      // Update pending DL LCID bitmap.
      auto& ue_cfg = u.at<ue_config_context>();
      pending_dl_ue_lcids.set(ue_cfg.ue_index * MAX_NOF_RB_LCIDS + lcid, buf_st.value() > 0);
    }
    on_single_channel_buf_st_update(
        u, lc_mapper.active(lc_rid), lc_mapper.dl_slice_id(lc_rid), buf_st.value(), prev_buf_st);
  }
}

void logical_channel_system::remove_lcg(soa::row_id                       ue_rid,
                                        ue_ul_channel_context&            ue_ctx,
                                        lcg_id_t                          lcgid,
                                        ul_logical_channel_group_context& ue_lcg)
{
  ocudu_assert(ue_lcg.lc_count == 0, "There are still LCIDs in this LCG");

  // Detach LCG from QoS tracking.
  if (ue_lcg.qos_row.has_value()) {
    qos_lcgs.erase(*ue_lcg.qos_row);
  }

  // Detach LCG from RAN slice.
  deregister_lcg_ran_slice(ue_rid, lcgid);

  // Delete LCG context.
  ue_ctx.lcgs.erase(lcgid);
}

void logical_channel_system::on_single_channel_buf_st_update(ue_row&                              u,
                                                             bool                                 channel_active,
                                                             const std::optional<ran_slice_id_t>& slice_id,
                                                             unsigned                             new_buf_st,
                                                             unsigned                             prev_buf_st)
{
  if (not slice_id.has_value() or not channel_active) {
    // No slice associated with this logical channel or it is inactive.
    return;
  }
  auto& ue_ctx = u.at<ue_context>();
  if (ue_ctx.fallback_state) {
    // In fallback mode, slices are disabled.
    return;
  }

  // In case this logical channel has a RAN slice associated, (differentially) update the pending bytes for the slice.
  auto&      slice_pending_bytes     = ue_ctx.pending_bytes_per_slice.find(*slice_id)->second;
  const auto prev_buf_st_incl_subhdr = get_mac_sdu_required_bytes(prev_buf_st);
  const auto new_buf_st_incl_subhdr  = get_mac_sdu_required_bytes(new_buf_st);
  ocudu_sanity_check(slice_pending_bytes + new_buf_st_incl_subhdr >= prev_buf_st_incl_subhdr,
                     "Invalid slice pending bytes");
  const auto prev_slice_pending_bytes = slice_pending_bytes;
  slice_pending_bytes += new_buf_st_incl_subhdr - prev_buf_st_incl_subhdr;

  if ((prev_slice_pending_bytes > 0) != (slice_pending_bytes > 0)) {
    // zero crossing detected. Update the bitmap of UEs with pending data for a given slice.
    auto& ue_cfg = u.at<ue_config_context>();
    slices.at(*slice_id).pending_dl_ues.set(ue_cfg.ue_index, slice_pending_bytes > 0);
  }
}

void logical_channel_system::on_single_lcg_buf_st_update(ue_row&                              u,
                                                         bool                                 channel_active,
                                                         lcg_id_t                             lcgid,
                                                         const std::optional<ran_slice_id_t>& slice_id,
                                                         unsigned                             new_buf_st,
                                                         unsigned                             prev_buf_st)
{
  if (not slice_id.has_value() or not channel_active or u.at<ue_context>().fallback_state) {
    // No slice associated with this logical channel group, the logical channel group is inactive or the UE is in
    // fallback.
    return;
  }
  auto& ue_ctx = u.at<ue_ul_context>();

  // In case this logical channel has a RAN slice associated, (differentially) update the pending bytes for the slice.
  auto&      slice_pending_bytes     = ue_ctx.pending_bytes_per_slice.find(*slice_id)->second;
  const auto prev_buf_st_incl_subhdr = get_mac_sdu_required_bytes(add_upper_layer_header_bytes(lcgid, prev_buf_st));
  const auto new_buf_st_incl_subhdr  = get_mac_sdu_required_bytes(add_upper_layer_header_bytes(lcgid, new_buf_st));
  ocudu_assert(slice_pending_bytes + new_buf_st_incl_subhdr >= prev_buf_st_incl_subhdr, "Invalid slice pending bytes");
  const auto prev_slice_pending_bytes = slice_pending_bytes;
  slice_pending_bytes += new_buf_st_incl_subhdr - prev_buf_st_incl_subhdr;

  if ((prev_slice_pending_bytes > 0) != (slice_pending_bytes > 0)) {
    // zero crossing detected. Update the slice UE with pending data bitmap.
    auto& ue_cfg = u.at<ue_config_context>();
    slices.at(*slice_id).pending_ul_ues.set(ue_cfg.ue_index, slice_pending_bytes > 0);
  }
}

bool logical_channel_system::handle_mac_ce_indication(soa::row_id ue_row_id, const mac_ce_info& ce)
{
  auto  u      = get_ue(ue_row_id);
  auto& ue_ctx = u.at<ue_context>();
  if (ce.ce_lcid == lcid_dl_sch_t::UE_CON_RES_ID) {
    // CON RES is a special case, as it needs to be always scheduled first.
    ue_ctx.pending_con_res_id = true;
    return true;
  }
  auto&                      ue_ch_ctx = u.at<ue_channel_context>();
  std::optional<soa::row_id> prev_ce_it;
  for (auto it = ue_ch_ctx.pending_ces; it.has_value();) {
    auto& ce_ctx = pending_ces.at<mac_ce_context>(it.value());
    if (ce.ce_lcid == lcid_dl_sch_t::TA_CMD and ce_ctx.info.ce_lcid == lcid_dl_sch_t::TA_CMD) {
      // Overwrite previous TA CMD CE.
      // Note: Size of TA CMD CE is fixed, so no need to update pending CE bytes.
      ce_ctx.info.ce_payload = ce.ce_payload;
      return true;
    }
    prev_ce_it = it;
    it         = ce_ctx.next_ue_ce;
  }
  // New CE, append to the list.
  auto ce_rid = pending_ces.insert(mac_ce_context{ce, std::nullopt});
  if (prev_ce_it.has_value()) {
    // Append CE to the end of the list.
    pending_ces.at<mac_ce_context>(prev_ce_it.value()).next_ue_ce = ce_rid;
  } else {
    // New head of the list.
    ue_ch_ctx.pending_ces = ce_rid;
  }
  // Update sum of pending CE bytes.
  ue_ctx.pending_ce_bytes += ce.ce_lcid.is_var_len_ce() ? get_mac_sdu_required_bytes(ce.ce_lcid.sizeof_ce())
                                                        : FIXED_SIZED_MAC_CE_SUBHEADER_SIZE + ce.ce_lcid.sizeof_ce();
  ues_with_pending_ces.set(u.at<ue_config_context>().ue_index, true);
  return true;
}

void logical_channel_system::handle_bsr_indication(soa::row_id ue_row_id, const ul_bsr_indication_message& msg)
{
  // This is the maximum bounded value that can be reported by the BSR, as per TS 38.321, Table 6.1.3.1-1.
  static constexpr unsigned max_short_bsr = 150000;
  // We apply this limit to avoid potential overflows.
  static constexpr unsigned max_buffer_status = 1U << 24U;

  auto  u     = get_ue(ue_row_id);
  auto& ue_ch = u.at<ue_ul_channel_context>();

  for (const auto& lcg_report : msg.reported_lcgs) {
    // In the scheduler, we use 300000 to indicate the max (unbounded) S-BSR value; therefore, if the short or short
    // truncated BSRs report a value that is > 150000 and the current status of the lcg_id is > 150000, we can't
    // update buf_st, as the actual UE BSR (buf_st) could be much bigger than the max (unbounded) S-BSR reported
    // value.
    auto it = ue_ch.lcgs.find(lcg_report.lcg_id);
    if (it == ue_ch.lcgs.end()) {
      // LCG-ID not configured, ignore. Should we log this?
      continue;
    }
    auto& lcg = it->second;
    if (msg.type == bsr_format::LONG_BSR or msg.type == bsr_format::LONG_TRUNC_BSR or lcg.buf_st <= max_short_bsr or
        lcg_report.nof_bytes <= max_short_bsr) {
      auto prev_buf_st = lcg.buf_st;
      lcg.buf_st       = std::min(lcg_report.nof_bytes, max_buffer_status);
      if (lcg.qos_row.has_value()) {
        qos_lcgs.at<lcg_qos_context>(*lcg.qos_row).sched_bytes_accum = 0;
      }
      on_single_lcg_buf_st_update(u, lcg.active, it->first, lcg.slice_id, lcg.buf_st, prev_buf_st);
    }
  }
}

unsigned logical_channel_system::total_dl_pending_bytes(const const_ue_row& u) const
{
  unsigned bytes = pending_ce_bytes(u);
  for (lcid_t lcid : u.at<ue_channel_context>().sorted_channels) {
    auto        lc_it  = u.at<ue_channel_context>().channels.find(lcid);
    soa::row_id lc_rid = lc_it->second;
    bytes += pending_bytes(lc_rid);
  }
  return bytes;
}

lcid_t logical_channel_system::get_max_prio_lcid(const const_ue_row& u) const
{
  for (const auto lcid : u.at<ue_channel_context>().sorted_channels) {
    auto lc_rid = lc_mapper.get_row_id(u.at<ue_config_context>().ue_index, lcid);
    if (lc_mapper.active(lc_rid) and lc_mapper.dl_buf_st(lc_rid).value() > 0) {
      return lcid;
    }
  }
  return INVALID_LCID;
}

unsigned
logical_channel_system::allocate_mac_sdu(soa::row_id ue_rid, dl_msg_lc_info& subpdu, lcid_t lcid, unsigned rem_bytes)
{
  ocudu_sanity_check(lcid < MAX_NOF_RB_LCIDS, "Max LCID value 32 exceeded");
  const unsigned min_bytes_needed = get_mac_sdu_with_subhdr_and_rlc_hdr_estim(lcid, 1);
  if (rem_bytes < min_bytes_needed) {
    // There is no space even for a minimal MAC SDU.
    return 0;
  }
  auto        u         = get_ue(ue_rid);
  auto&       ue_ch_ctx = u.at<ue_channel_context>();
  soa::row_id lc_rid    = ue_ch_ctx.channels.at(lcid);

  unsigned lch_bytes_and_subhr = pending_bytes(lc_rid);
  if (lch_bytes_and_subhr == 0) {
    return 0;
  }
  // This ensures that regardless of the pending bytes, enough space is allocated for the RLC overhead.
  lch_bytes_and_subhr = std::max(lch_bytes_and_subhr, min_bytes_needed);

  // Account for available space and MAC subheader to decide the number of bytes to allocate.
  unsigned alloc_bytes = std::min(rem_bytes, lch_bytes_and_subhr);

  // Allocate all leftover bytes in following cases:
  // - [Implementation-defined] If \c leftover_bytes is less than 5 bytes, as it is unlikely they will be used for
  // another SDU.
  const unsigned            leftover_bytes     = rem_bytes - alloc_bytes;
  constexpr static unsigned MIN_LEFTOVER_BYTES = 5;
  if (leftover_bytes > 0 and (leftover_bytes <= MIN_LEFTOVER_BYTES)) {
    alloc_bytes += leftover_bytes;
  }
  if (alloc_bytes == MAC_SDU_SUBHEADER_LENGTH_THRES + MIN_MAC_SDU_SUBHEADER_SIZE) {
    // Avoid invalid combination of MAC subPDU and subheader size.
    alloc_bytes--;
  }

  // Compute MAC SDU size without MAC subheader.
  const unsigned sdu_size = get_mac_sdu_size(alloc_bytes);

  // Update DL Buffer Status to avoid reallocating the same LCID bytes.
  auto&          buf_st           = lc_mapper.dl_buf_st(lc_rid);
  const unsigned last_sched_bytes = std::min(sdu_size, buf_st.value());
  auto           new_buf_st       = buf_st.value() - last_sched_bytes;
  if (lcid != LCID_SRB0 and new_buf_st > 0) {
    // Allocation was not enough to empty the logical channel. In this specific case, we add some bytes to account
    // for the RLC segmentation overhead.
    // Note: This update is only relevant for PDSCH allocations for slots > slot_tx. For the case of PDSCH
    // slot==slot_tx, there will be an RLC Buffer Occupancy update right away, which will set a new buffer value.
    new_buf_st += RLC_SEGMENTATION_OVERHEAD;
  }
  auto prev_buf_st = buf_st.value();
  buf_st           = units::bytes{new_buf_st};
  if ((prev_buf_st > 0) != (new_buf_st > 0)) {
    // Update pending DL LCID bitmap.
    auto& ue_cfg = u.at<ue_config_context>();
    pending_dl_ue_lcids.set(ue_cfg.ue_index * MAX_NOF_RB_LCIDS + lcid, new_buf_st > 0);
  }
  on_single_channel_buf_st_update(u, lc_mapper.active(lc_rid), lc_mapper.dl_slice_id(lc_rid), new_buf_st, prev_buf_st);
  if (unsigned* last_sched_ptr = lc_mapper.last_sched_bytes(lc_rid); last_sched_ptr != nullptr) {
    // In case of QoS tracking is activated, set the last scheduled bytes.
    *last_sched_ptr = last_sched_bytes;
  }

  // Set subPDU info.
  subpdu.lcid        = lcid;
  subpdu.sched_bytes = sdu_size;

  return alloc_bytes;
}

unsigned
logical_channel_system::allocate_mac_sdu(soa::row_id ue_rid, dl_msg_lc_info& subpdu, unsigned rem_bytes, lcid_t lcid)
{
  subpdu.lcid        = lcid_dl_sch_t::MIN_RESERVED;
  subpdu.sched_bytes = 0;

  lcid_t lcid_with_prio = lcid == INVALID_LCID ? get_max_prio_lcid(get_ue(ue_rid)) : lcid;
  if (lcid_with_prio == INVALID_LCID) {
    return 0;
  }

  // Update Buffer Status of allocated LCID.
  return allocate_mac_sdu(ue_rid, subpdu, lcid_with_prio, rem_bytes);
}

unsigned logical_channel_system::allocate_mac_ce(soa::row_id ue_rid, dl_msg_lc_info& subpdu, unsigned rem_bytes)
{
  subpdu.lcid        = lcid_dl_sch_t::MIN_RESERVED;
  subpdu.sched_bytes = 0;

  // Allocate ConRes CE ID if pending.
  unsigned alloc_bytes = allocate_ue_con_res_id_mac_ce(ue_rid, subpdu, rem_bytes);
  if (alloc_bytes > 0) {
    return alloc_bytes;
  }

  auto  u         = get_ue(ue_rid);
  auto& ue_ch_ctx = u.at<ue_channel_context>();
  if (not ue_ch_ctx.pending_ces.has_value()) {
    return 0;
  }
  soa::row_id         ce_row_id = *ue_ch_ctx.pending_ces;
  const auto&         ce_node   = pending_ces.at<mac_ce_context>(ce_row_id);
  const lcid_dl_sch_t lcid      = ce_node.info.ce_lcid;

  // Derive space needed for CE subheader + payload.
  const unsigned ce_size = lcid.sizeof_ce();
  if (lcid.is_var_len_ce()) {
    alloc_bytes = get_mac_sdu_required_bytes(ce_size);
  } else {
    alloc_bytes = ce_size + FIXED_SIZED_MAC_CE_SUBHEADER_SIZE;
  }

  // Verify there is space for both MAC CE and subheader.
  if (rem_bytes < alloc_bytes) {
    return 0;
  }

  subpdu.lcid        = lcid;
  subpdu.sched_bytes = ce_size;
  subpdu.ce_payload  = ce_node.info.ce_payload;

  // Pop MAC CE.
  ue_ch_ctx.pending_ces = ce_node.next_ue_ce;
  // Update sum of pending CE bytes.
  u.at<ue_context>().pending_ce_bytes -= alloc_bytes;
  pending_ces.erase(ce_row_id);
  if (not ue_ch_ctx.pending_ces.has_value()) {
    ues_with_pending_ces.set(u.at<ue_config_context>().ue_index, false);
  }

  return alloc_bytes;
}

unsigned
logical_channel_system::allocate_ue_con_res_id_mac_ce(soa::row_id ue_rid, dl_msg_lc_info& subpdu, unsigned rem_bytes)
{
  auto& ue_ctx = get_ue(ue_rid).at<ue_context>();
  if (not ue_ctx.pending_con_res_id) {
    return 0;
  }

  subpdu.lcid        = lcid_dl_sch_t::MIN_RESERVED;
  subpdu.sched_bytes = 0;

  static const auto ce_size     = lcid_dl_sch_t{lcid_dl_sch_t::UE_CON_RES_ID}.sizeof_ce();
  unsigned          alloc_bytes = ce_size + FIXED_SIZED_MAC_CE_SUBHEADER_SIZE;

  // Verify there is space for both MAC CE and subheader.
  if (rem_bytes < alloc_bytes) {
    alloc_bytes = 0;
  }

  if (alloc_bytes > 0) {
    ue_ctx.pending_con_res_id = false;

    subpdu.lcid        = lcid_dl_sch_t{lcid_dl_sch_t::UE_CON_RES_ID};
    subpdu.sched_bytes = ce_size;
  }

  return alloc_bytes;
}

// class logical_channel_system::ue_logical_channel_repository

void ue_logical_channel_repository::configure(logical_channel_config_list_ptr log_channels_configs)
{
  parent->configure(ue_row_id, log_channels_configs);
}

void ue_logical_channel_repository::deactivate()
{
  parent->deactivate(ue_row_id);
}

void ue_logical_channel_repository::set_lcid_ran_slice(lcid_t lcid, ran_slice_id_t slice_id)
{
  parent->set_lcid_ran_slice(ue_row_id, lcid, slice_id);
}

void ue_logical_channel_repository::set_lcg_ran_slice(lcg_id_t lcgid, ran_slice_id_t slice_id)
{
  parent->set_lcg_ran_slice(ue_row_id, lcgid, slice_id);
}

void ue_logical_channel_repository::reset_lcid_ran_slice(lcid_t lcid)
{
  parent->deregister_lc_ran_slice(ue_row_id, lcid);
}

void ue_logical_channel_repository::reset_lcg_ran_slice(lcg_id_t lcgid)
{
  parent->deregister_lcg_ran_slice(ue_row_id, lcgid);
}

unsigned ue_logical_channel_repository::dl_pending_bytes() const
{
  auto u = get_ue_row();
  if (u.at<ue_context>().fallback_state) {
    return pending_con_res_ce_bytes() + pending_bytes(LCID_SRB0) + pending_bytes(LCID_SRB1);
  }
  unsigned bytes = pending_ce_bytes();
  auto&    ue_ch = u.at<ue_channel_context>();
  for (lcid_t lcid : ue_ch.sorted_channels) {
    bytes += lcid != LCID_SRB0 ? pending_bytes(lcid) : 0;
  }
  return bytes;
}

unsigned ue_logical_channel_repository::ul_pending_bytes() const
{
  auto u = get_ue_row();
  if (u.at<ue_context>().fallback_state) {
    return pending_bytes(uint_to_lcg_id(0U));
  }
  unsigned bytes = 0;
  for (const auto& [lcgid, ul_ch_ctx] : u.at<ue_ul_channel_context>().lcgs) {
    bytes += pending_bytes(lcgid);
  }
  return bytes;
}

double ue_logical_channel_repository::average_dl_bit_rate(lcid_t lcid) const
{
  if (parent->lc_mapper.contains(ue_index, lcid)) {
    soa::row_id lc_rid  = parent->lc_mapper.get_row_id(ue_index, lcid);
    auto        qos_row = parent->lc_mapper.qos_row(lc_rid);
    if (qos_row.has_value()) {
      auto u = get_ue_row();
      return parent->lc_mapper.avg_dl_bits_per_slot(lc_rid) * u.at<ue_config_context>().slots_per_msec * 1000;
    }
  }
  return 0.0;
}

double ue_logical_channel_repository::average_ul_bit_rate(lcg_id_t lcgid) const
{
  auto  u      = get_ue_row();
  auto* ch_ctx = logical_channel_system::find_active_lcg_ctx(u, lcgid);
  auto* qos_ch = (ch_ctx != nullptr and ch_ctx->qos_row.has_value())
                     ? &parent->qos_lcgs.row(ch_ctx->qos_row.value()).at<logical_channel_system::lcg_qos_context>()
                     : nullptr;
  return qos_ch != nullptr
             ? qos_ch->ul_avg_bytes_per_slot.average() * 8 * u.at<ue_config_context>().slots_per_msec * 1000
             : 0.0;
}

void ue_logical_channel_repository::handle_dl_buffer_status_indication(lcid_t     lcid,
                                                                       unsigned   buffer_status,
                                                                       slot_point hol_toa)
{
  parent->handle_dl_buffer_status_indication(ue_row_id, lcid, buffer_status, hol_toa);
}

bool ue_logical_channel_repository_view::handle_mac_ce_indication(const mac_ce_info& ce)
{
  return parent->handle_mac_ce_indication(ue_row_id, ce);
}

void ue_logical_channel_repository::handle_bsr_indication(const ul_bsr_indication_message& msg)
{
  parent->handle_bsr_indication(ue_row_id, msg);
}

void ue_logical_channel_repository::handle_sr_indication()
{
  parent->ues_with_pending_sr.set(ue_index, true);
}

unsigned ue_logical_channel_repository::allocate_mac_sdu(dl_msg_lc_info& lch_info, unsigned rem_bytes, lcid_t lcid)
{
  return parent->allocate_mac_sdu(ue_row_id, lch_info, rem_bytes, lcid);
}

unsigned ue_logical_channel_repository::allocate_mac_ce(dl_msg_lc_info& lch_info, unsigned rem_bytes)
{
  return parent->allocate_mac_ce(ue_row_id, lch_info, rem_bytes);
}

unsigned ue_logical_channel_repository::allocate_ue_con_res_id_mac_ce(dl_msg_lc_info& lch_info, unsigned rem_bytes)
{
  return parent->allocate_ue_con_res_id_mac_ce(ue_row_id, lch_info, rem_bytes);
}

void ue_logical_channel_repository::handle_ul_grant(unsigned grant_size)
{
  // Reset any pending SR indication.
  reset_sr_indication();

  // Update estimates of logical channel bit rates.
  auto& lcgs = get_ue_row().at<ue_ul_channel_context>().lcgs;
  for (auto it = lcgs.begin(); it != lcgs.end() and grant_size > 0; ++it) {
    auto& lcg_ctx = it->second;
    if (lcg_ctx.qos_row.has_value() and lcg_ctx.active and lcg_ctx.buf_st > 0) {
      // This LCG has data to send and its QoS is being tracked.
      auto& qos_ctx = parent->qos_lcgs.at<logical_channel_system::lcg_qos_context>(lcg_ctx.qos_row.value());
      if (lcg_ctx.buf_st > qos_ctx.sched_bytes_accum) {
        // There is still data to send in this LCG, so we consider the last scheduled bytes for the average.
        unsigned bytes_sched = std::min(
            lcg_ctx.buf_st > qos_ctx.sched_bytes_accum ? lcg_ctx.buf_st - qos_ctx.sched_bytes_accum : 0, grant_size);
        qos_ctx.ul_last_sched_bytes += bytes_sched;
        qos_ctx.sched_bytes_accum += bytes_sched;
        grant_size -= bytes_sched;
      }
    }
  }
}

void ue_logical_channel_repository::reset_sr_indication()
{
  parent->ues_with_pending_sr.set(ue_index, false);
}

unsigned ocudu::allocate_mac_sdus(dl_msg_tb_info&                tb_info,
                                  ue_logical_channel_repository& lch_mng,
                                  unsigned                       total_tbs,
                                  lcid_t                         lcid)
{
  static constexpr unsigned min_mac_sdu_space = 4; // Needs to fit at least MAC SDU subheader and RLC header.
  unsigned                  rem_tbs           = total_tbs;

  // If we do not have enough bytes to fit MAC subheader, skip MAC SDU allocation.
  // Note: We assume upper layer accounts for its own subheaders when updating the buffer state.
  while (rem_tbs > min_mac_sdu_space and not tb_info.lc_chs_to_sched.full()) {
    dl_msg_lc_info subpdu;
    unsigned       alloc_bytes = lch_mng.allocate_mac_sdu(subpdu, rem_tbs, lcid);
    if (alloc_bytes == 0) {
      break;
    }

    // Add new subPDU.
    tb_info.lc_chs_to_sched.push_back(subpdu);

    // Update remaining space taking into account the MAC SDU subheader.
    rem_tbs -= alloc_bytes;
  }

  return total_tbs - rem_tbs;
}

unsigned ocudu::allocate_mac_ces(dl_msg_tb_info& tb_info, ue_logical_channel_repository& lch_mng, unsigned total_tbs)
{
  unsigned rem_tbs = total_tbs;

  while (lch_mng.has_pending_ces() and not tb_info.lc_chs_to_sched.full()) {
    dl_msg_lc_info subpdu;
    unsigned       alloc_bytes = lch_mng.allocate_mac_ce(subpdu, rem_tbs);
    if (alloc_bytes == 0) {
      break;
    }

    // Add new subPDU.
    tb_info.lc_chs_to_sched.push_back(subpdu);

    // Update remaining space taking into account the MAC CE subheader.
    rem_tbs -= alloc_bytes;
  }
  return total_tbs - rem_tbs;
}

unsigned ocudu::allocate_ue_con_res_id_mac_ce(dl_msg_tb_info&                tb_info,
                                              ue_logical_channel_repository& lch_mng,
                                              unsigned                       total_tbs)
{
  unsigned rem_tbs = total_tbs;

  if (not tb_info.lc_chs_to_sched.full()) {
    dl_msg_lc_info subpdu;
    unsigned       alloc_bytes = lch_mng.allocate_ue_con_res_id_mac_ce(subpdu, rem_tbs);
    if (alloc_bytes != 0) {
      // Add new subPDU.
      tb_info.lc_chs_to_sched.push_back(subpdu);

      // Update remaining space taking into account the MAC CE subheader.
      rem_tbs -= alloc_bytes;
    }
  }
  return total_tbs - rem_tbs;
}

unsigned ocudu::build_dl_fallback_transport_block_info(dl_msg_tb_info&                tb_info,
                                                       ue_logical_channel_repository& lch_mng,
                                                       unsigned                       tb_size_bytes)
{
  unsigned total_subpdu_bytes = 0;
  total_subpdu_bytes += allocate_ue_con_res_id_mac_ce(tb_info, lch_mng, tb_size_bytes);
  // Since SRB0 PDU cannot be segmented, skip SRB0 if remaining TB size is not enough to fit entire PDU.
  if (lch_mng.has_pending_bytes(LCID_SRB0) and
      ((tb_size_bytes - total_subpdu_bytes) >= lch_mng.pending_bytes(LCID_SRB0))) {
    total_subpdu_bytes += allocate_mac_sdus(tb_info, lch_mng, tb_size_bytes - total_subpdu_bytes, LCID_SRB0);
    return total_subpdu_bytes;
  }
  total_subpdu_bytes += allocate_mac_sdus(tb_info, lch_mng, tb_size_bytes - total_subpdu_bytes, LCID_SRB1);
  return total_subpdu_bytes;
}

unsigned ocudu::build_dl_transport_block_info(dl_msg_tb_info&                tb_info,
                                              ue_logical_channel_repository& lch_mng,
                                              unsigned                       tb_size_bytes,
                                              ran_slice_id_t                 slice_id)
{
  unsigned total_subpdu_bytes = 0;
  total_subpdu_bytes += allocate_mac_ces(tb_info, lch_mng, tb_size_bytes);
  for (const auto lcid : lch_mng.get_prioritized_logical_channels()) {
    if (lch_mng.get_slice_id(lcid) == slice_id) {
      total_subpdu_bytes += allocate_mac_sdus(tb_info, lch_mng, tb_size_bytes - total_subpdu_bytes, uint_to_lcid(lcid));
    }
  }
  return total_subpdu_bytes;
}
