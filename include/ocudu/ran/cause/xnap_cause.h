// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/ran/cause/common.h"
#include "fmt/format.h"
#include <variant>

namespace ocudu {

/// The XNAP radio network cause, see TS 38.423 section 9.2.3.2.
enum class xnap_cause_radio_network_t : uint8_t {
  cell_not_available = 0,
  ho_desirable_for_radio_reasons,
  ho_target_not_allowed,
  invalid_amf_set_id,
  no_radio_res_available_in_target_cell,
  partial_ho,
  reduce_load_in_serving_cell,
  res_optim_ho,
  time_crit_ho,
  txn_relo_coverall_expiry,
  txn_relo_cprep_expiry,
  unknown_guami_id,
  unknown_local_ng_ran_node_ue_xn_ap_id,
  inconsistent_remote_ng_ran_node_ue_xn_ap_id,
  encryption_and_or_integrity_protection_algorithms_not_supported,
  not_used_causes_value_neg1,
  multiple_pdu_session_id_instances,
  unknown_pdu_session_id,
  unknown_qos_flow_id,
  multiple_qos_flow_id_instances,
  switch_off_ongoing,
  not_supported_5qi_value,
  txn_d_coverall_expiry,
  txn_d_cprep_expiry,
  action_desirable_for_radio_reasons,
  reduce_load,
  res_optim,
  time_crit_action,
  target_not_allowed,
  no_radio_res_available,
  invalid_qos_combination,
  encryption_algorithms_not_supported,
  proc_cancelled,
  rrm_purpose,
  improve_user_bit_rate,
  user_inactivity,
  radio_conn_with_ue_lost,
  fail_in_the_radio_interface_proc,
  bearer_option_not_supported,
  up_integrity_protection_not_possible,
  up_confidentiality_protection_not_possible,
  res_not_available_for_the_slice_s,
  ue_max_ip_data_rate_reason,
  cp_integrity_protection_fail,
  up_integrity_protection_fail,
  slice_not_supported_by_ng_ran,
  mn_mob,
  sn_mob,
  count_reaches_max_value,
  unknown_old_ng_ran_node_ue_xn_ap_id,
  pdcp_overload,
  drb_id_not_available,
  unspecified,
  ue_context_id_not_known,
  non_relocation_of_context,
  cho_cpc_res_tobechanged,
  rsn_not_available_for_the_up,
  npn_access_denied,
  report_characteristics_empty,
  existing_meas_id,
  meas_temporarily_not_available,
  meas_not_supported_for_the_obj,
  ue_pwr_saving,
  not_existing_ng_ran_node2_meas_id,
  insufficient_ue_cap,
  normal_release,
  value_out_of_allowed_range,
  scg_activation_deactivation_fail,
  scg_deactivation_fail_due_to_data_tx,
  ssb_not_available,
  ltm_triggered,
  no_backhaul_res,
  miab_node_not_authorized,
  iab_not_authorized,
};

/// The XNAP transport cause, see TS 38.423 section 9.2.3.2.
enum class xnap_cause_transport_t : uint8_t { transport_res_unavailable = 0, unspecified };

/// The XNAP misc cause, see TS 38.423 section 9.2.3.2.
enum class xnap_cause_misc_t : uint8_t {
  ctrl_processing_overload = 0,
  hardware_fail,
  o_and_m_intervention,
  not_enough_user_plane_processing_res,
  unspecified
};

/// The XNAP cause to indicate the reason for a particular event, see TS 38.423 section 9.2.3.2.
/// The XNAP cause is a union of the radio network cause, transport cause, nas cause, protocol cause and misc cause.
using xnap_cause_t =
    std::variant<xnap_cause_radio_network_t, xnap_cause_transport_t, cause_protocol_t, xnap_cause_misc_t>;

} // namespace ocudu

namespace fmt {

template <>
struct formatter<ocudu::xnap_cause_radio_network_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::xnap_cause_radio_network_t& cause, FormatContext& ctx) const
  {
    if (cause == ocudu::xnap_cause_radio_network_t::cell_not_available) {
      return format_to(ctx.out(), "cell_not_available");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ho_desirable_for_radio_reasons) {
      return format_to(ctx.out(), "ho_desirable_for_radio_reasons");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ho_target_not_allowed) {
      return format_to(ctx.out(), "ho_target_not_allowed");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::invalid_amf_set_id) {
      return format_to(ctx.out(), "invalid_amf_set_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::no_radio_res_available_in_target_cell) {
      return format_to(ctx.out(), "no_radio_res_available_in_target_cell");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::partial_ho) {
      return format_to(ctx.out(), "partial_ho");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::partial_ho) {
      return format_to(ctx.out(), "partial_ho");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::reduce_load_in_serving_cell) {
      return format_to(ctx.out(), "reduce_load_in_serving_cell");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::res_optim_ho) {
      return format_to(ctx.out(), "res_optim_ho");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::time_crit_ho) {
      return format_to(ctx.out(), "time_crit_ho");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::txn_relo_coverall_expiry) {
      return format_to(ctx.out(), "txn_relo_coverall_expiry");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::txn_relo_cprep_expiry) {
      return format_to(ctx.out(), "txn_relo_cprep_expiry");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::unknown_guami_id) {
      return format_to(ctx.out(), "unknown_guami_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::unknown_local_ng_ran_node_ue_xn_ap_id) {
      return format_to(ctx.out(), "unknown_local_ng_ran_node_ue_xn_ap_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::inconsistent_remote_ng_ran_node_ue_xn_ap_id) {
      return format_to(ctx.out(), "inconsistent_remote_ng_ran_node_ue_xn_ap_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::encryption_and_or_integrity_protection_algorithms_not_supported) {
      return format_to(ctx.out(), "encryption_and_or_integrity_protection_algorithms_not_supported");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::not_used_causes_value_neg1) {
      return format_to(ctx.out(), "not_used_causes_value_neg1");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::multiple_pdu_session_id_instances) {
      return format_to(ctx.out(), "multiple_pdu_session_id_instances");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::unknown_pdu_session_id) {
      return format_to(ctx.out(), "unknown_pdu_session_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::unknown_qos_flow_id) {
      return format_to(ctx.out(), "unknown_qos_flow_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::multiple_qos_flow_id_instances) {
      return format_to(ctx.out(), "multiple_qos_flow_id_instances");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::switch_off_ongoing) {
      return format_to(ctx.out(), "switch_off_ongoing");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::not_supported_5qi_value) {
      return format_to(ctx.out(), "not_supported_5qi_value");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::txn_d_coverall_expiry) {
      return format_to(ctx.out(), "txn_d_coverall_expiry");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::txn_d_cprep_expiry) {
      return format_to(ctx.out(), "txn_d_cprep_expiry");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::action_desirable_for_radio_reasons) {
      return format_to(ctx.out(), "action_desirable_for_radio_reasons");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::reduce_load) {
      return format_to(ctx.out(), "reduce_load");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::res_optim) {
      return format_to(ctx.out(), "res_optim");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::time_crit_action) {
      return format_to(ctx.out(), "time_crit_action");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::target_not_allowed) {
      return format_to(ctx.out(), "target_not_allowed");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::no_radio_res_available) {
      return format_to(ctx.out(), "no_radio_res_available");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::invalid_qos_combination) {
      return format_to(ctx.out(), "invalid_qos_combination");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::encryption_algorithms_not_supported) {
      return format_to(ctx.out(), "encryption_algorithms_not_supported");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::proc_cancelled) {
      return format_to(ctx.out(), "proc_cancelled");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::rrm_purpose) {
      return format_to(ctx.out(), "rrm_purpose");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::improve_user_bit_rate) {
      return format_to(ctx.out(), "improve_user_bit_rate");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::user_inactivity) {
      return format_to(ctx.out(), "user_inactivity");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::radio_conn_with_ue_lost) {
      return format_to(ctx.out(), "radio_conn_with_ue_lost");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::fail_in_the_radio_interface_proc) {
      return format_to(ctx.out(), "fail_in_the_radio_interface_proc");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::bearer_option_not_supported) {
      return format_to(ctx.out(), "bearer_option_not_supported");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::up_integrity_protection_not_possible) {
      return format_to(ctx.out(), "up_integrity_protection_not_possible");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::up_confidentiality_protection_not_possible) {
      return format_to(ctx.out(), "up_confidentiality_protection_not_possible");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::res_not_available_for_the_slice_s) {
      return format_to(ctx.out(), "res_not_available_for_the_slice_s");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ue_max_ip_data_rate_reason) {
      return format_to(ctx.out(), "ue_max_ip_data_rate_reason");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::cp_integrity_protection_fail) {
      return format_to(ctx.out(), "cp_integrity_protection_fail");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::up_integrity_protection_fail) {
      return format_to(ctx.out(), "up_integrity_protection_fail");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::slice_not_supported_by_ng_ran) {
      return format_to(ctx.out(), "slice_not_supported_by_ng_ran");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::mn_mob) {
      return format_to(ctx.out(), "mn_mob");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::sn_mob) {
      return format_to(ctx.out(), "sn_mob");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::count_reaches_max_value) {
      return format_to(ctx.out(), "count_reaches_max_value");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::unknown_old_ng_ran_node_ue_xn_ap_id) {
      return format_to(ctx.out(), "unknown_old_ng_ran_node_ue_xn_ap_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::pdcp_overload) {
      return format_to(ctx.out(), "pdcp_overload");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::drb_id_not_available) {
      return format_to(ctx.out(), "drb_id_not_available");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::unspecified) {
      return format_to(ctx.out(), "unspecified");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ue_context_id_not_known) {
      return format_to(ctx.out(), "ue_context_id_not_known");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::non_relocation_of_context) {
      return format_to(ctx.out(), "non_relocation_of_context");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::cho_cpc_res_tobechanged) {
      return format_to(ctx.out(), "cho_cpc_res_tobechanged");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::rsn_not_available_for_the_up) {
      return format_to(ctx.out(), "rsn_not_available_for_the_up");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::npn_access_denied) {
      return format_to(ctx.out(), "npn_access_denied");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::report_characteristics_empty) {
      return format_to(ctx.out(), "report_characteristics_empty");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::existing_meas_id) {
      return format_to(ctx.out(), "existing_meas_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::meas_temporarily_not_available) {
      return format_to(ctx.out(), "meas_temporarily_not_available");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::meas_not_supported_for_the_obj) {
      return format_to(ctx.out(), "meas_not_supported_for_the_obj");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ue_pwr_saving) {
      return format_to(ctx.out(), "ue_pwr_saving");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::not_existing_ng_ran_node2_meas_id) {
      return format_to(ctx.out(), "not_existing_ng_ran_node2_meas_id");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::insufficient_ue_cap) {
      return format_to(ctx.out(), "insufficient_ue_cap");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::normal_release) {
      return format_to(ctx.out(), "normal_release");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::value_out_of_allowed_range) {
      return format_to(ctx.out(), "value_out_of_allowed_range");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::scg_activation_deactivation_fail) {
      return format_to(ctx.out(), "scg_activation_deactivation_fail");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::scg_deactivation_fail_due_to_data_tx) {
      return format_to(ctx.out(), "scg_deactivation_fail_due_to_data_tx");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ssb_not_available) {
      return format_to(ctx.out(), "ssb_not_available");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::ltm_triggered) {
      return format_to(ctx.out(), "ltm_triggered");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::no_backhaul_res) {
      return format_to(ctx.out(), "no_backhaul_res");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::miab_node_not_authorized) {
      return format_to(ctx.out(), "miab_node_not_authorized");
    }
    if (cause == ocudu::xnap_cause_radio_network_t::iab_not_authorized) {
      return format_to(ctx.out(), "iab_not_authorized");
    }

    return format_to(ctx.out(), "unknown");
  }
};

template <>
struct formatter<ocudu::xnap_cause_transport_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::xnap_cause_transport_t& cause, FormatContext& ctx) const
  {
    if (cause == ocudu::xnap_cause_transport_t::transport_res_unavailable) {
      return format_to(ctx.out(), "transport_res_unavailable");
    }
    if (cause == ocudu::xnap_cause_transport_t::unspecified) {
      return format_to(ctx.out(), "unspecified");
    }

    return format_to(ctx.out(), "unknown");
  }
};

template <>
struct formatter<ocudu::xnap_cause_misc_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const ocudu::xnap_cause_misc_t& cause, FormatContext& ctx) const
  {
    if (cause == ocudu::xnap_cause_misc_t::ctrl_processing_overload) {
      return format_to(ctx.out(), "ctrl_processing_overload");
    }
    if (cause == ocudu::xnap_cause_misc_t::hardware_fail) {
      return format_to(ctx.out(), "hardware_fail");
    }
    if (cause == ocudu::xnap_cause_misc_t::o_and_m_intervention) {
      return format_to(ctx.out(), "o_and_m_intervention");
    }
    if (cause == ocudu::xnap_cause_misc_t::not_enough_user_plane_processing_res) {
      return format_to(ctx.out(), "not_enough_user_plane_processing_res");
    }
    if (cause == ocudu::xnap_cause_misc_t::unspecified) {
      return format_to(ctx.out(), "unspecified");
    }

    return format_to(ctx.out(), "unknown");
  }
};

template <>
struct formatter<ocudu::xnap_cause_t> {
  template <typename ParseContext>
  auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(ocudu::xnap_cause_t o, FormatContext& ctx) const
  {
    if (const auto* result = std::get_if<ocudu::xnap_cause_radio_network_t>(&o)) {
      return format_to(ctx.out(), "radio_network-{}", *result);
    }
    if (const auto* result = std::get_if<ocudu::xnap_cause_transport_t>(&o)) {
      return format_to(ctx.out(), "transport-{}", *result);
    }
    if (const auto* result = std::get_if<ocudu::cause_protocol_t>(&o)) {
      return format_to(ctx.out(), "protocol-{}", *result);
    }
    return format_to(ctx.out(), "misc-{}", std::get<ocudu::xnap_cause_misc_t>(o));
  }
};

} // namespace fmt
