// SPDX-FileCopyrightText: Copyright (C) 2021-2026 Software Radio Systems Limited
// SPDX-License-Identifier: BSD-3-Clause-Open-MPI
// Portions of this file may implement 3GPP specifications, which may be subject to additional licensing requirements.

#pragma once

#include "ocudu/cu_cp/cu_cp_types.h"
#include "ocudu/ran/cause/ngap_cause.h"
#include "ocudu/ran/cause/xnap_cause.h"

namespace ocudu::ocucp {

struct cu_cp_data_forwarding_resp_drb_item {
  drb_id_t                               drb_id = drb_id_t::drb1;
  std::optional<up_transport_layer_info> dl_forwarding_up_tnl;
  std::optional<up_transport_layer_info> ul_forwarding_up_tnl;
};

struct cu_cp_qos_flow_with_data_forwarding_item {
  qos_flow_id_t qos_flow_id = qos_flow_id_t::invalid;
  // NG handover specific field.
  std::optional<bool> data_forwarding_accepted;
};

struct cu_cp_qos_flow_with_cause_item {
  qos_flow_id_t                                           qos_flow_id = qos_flow_id_t::invalid;
  std::variant<ngap_cause_t, std::optional<xnap_cause_t>> cause;
};

struct cu_cp_data_forwarding_info_from_target_ng_ran_node {
  // XNAP handover specific fields.
  std::vector<qos_flow_id_t>             qos_flows_accepted_for_data_forwarding_list;
  std::optional<up_transport_layer_info> pdu_session_level_dl_data_forwarding_info;
  std::optional<up_transport_layer_info> pdu_session_level_ul_data_forwarding_info;

  // Common fields.
  std::vector<cu_cp_data_forwarding_resp_drb_item> data_forwarding_resp_drb_item_list;
};

struct cu_cp_ng_pdu_session_res_admitted_item {
  pdu_session_id_t                                      pdu_session_id = pdu_session_id_t::invalid;
  up_transport_layer_info                               dl_ngu_up_tnl_info;
  std::optional<up_transport_layer_info>                dl_forwarding_up_tnl_info;
  std::optional<security_result_t>                      security_result;
  std::vector<cu_cp_qos_flow_with_data_forwarding_item> qos_flows_setup_list;
  std::vector<cu_cp_qos_flow_with_cause_item>           qos_flows_failed_to_setup_list;
  cu_cp_data_forwarding_info_from_target_ng_ran_node    data_forwarding_info_from_target;
};

struct cu_cp_xn_pdu_session_res_admitted_item {
  pdu_session_id_t                                                  pdu_session_id = pdu_session_id_t::invalid;
  bool                                                              dl_ngu_tnl_info_unchanged = false;
  std::vector<cu_cp_qos_flow_with_data_forwarding_item>             qos_flows_setup_list;
  std::vector<cu_cp_qos_flow_with_cause_item>                       qos_flows_failed_to_setup_list;
  std::optional<cu_cp_data_forwarding_info_from_target_ng_ran_node> data_forwarding_info_from_target;
};

using cu_cp_pdu_session_res_admitted_item =
    std::variant<cu_cp_ng_pdu_session_res_admitted_item, cu_cp_xn_pdu_session_res_admitted_item>;

struct cu_cp_pdu_session_with_cause_item {
  pdu_session_id_t                         pdu_session_id = pdu_session_id_t::invalid;
  std::variant<ngap_cause_t, xnap_cause_t> cause;
};

struct cu_cp_handover_request_ack {
  ue_index_t                                       ue_index = ue_index_t::invalid;
  std::vector<cu_cp_pdu_session_res_admitted_item> pdu_session_res_admitted_list;
  std::vector<cu_cp_pdu_session_with_cause_item>   pdu_session_failed_to_setup_list;
  byte_buffer                                      rrc_handover_command;
};

struct cu_cp_handover_request_failure {
  ue_index_t                               ue_index = ue_index_t::invalid;
  std::variant<ngap_cause_t, xnap_cause_t> cause;
};

using cu_cp_handover_resource_allocation_response =
    std::variant<cu_cp_handover_request_ack, cu_cp_handover_request_failure>;

} // namespace ocudu::ocucp
